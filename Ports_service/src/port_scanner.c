#include "port_scanner.h"
#include "port_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct {
    int puerto_actual;
    int puerto_final;
    pthread_mutex_t lock;
    GHashTable *tabla;
} ContextoEscaneo;

// Intenta conectar TCP a localhost:puerto, timeout 100 ms
static int puerto_abierto(int puerto) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 0;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(puerto),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };
    struct timeval tv = { 0, 100000 }; // 100 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    int ok = (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sockfd);
    return ok;
}

// Rutina que ejecuta cada hilo trabajador
static void* trabajador(void *arg) {
    ContextoEscaneo *ctx = (ContextoEscaneo*)arg;
    while (1) {
        pthread_mutex_lock(&ctx->lock);
        if (ctx->puerto_actual > ctx->puerto_final) {
            pthread_mutex_unlock(&ctx->lock);
            break;
        }
        int puerto = ctx->puerto_actual++;
        pthread_mutex_unlock(&ctx->lock);

        if (puerto_abierto(puerto)) {
            const char *svc = buscar_servicio(ctx->tabla, puerto);
            if (svc) {
                printf("PUERTO ABIERTO: %d → Servicio: %s\n", puerto, svc);
            } else {
                printf("⚠️ PUERTO ABIERTO: %d → DESCONOCIDO (SOSPECHOSO)\n", puerto);
            }
        }
    }
    return NULL;
}

void escanear_puertos(GHashTable *tabla, int inicio, int fin) {
    pthread_t hilos[NUM_HILOS];
    ContextoEscaneo ctx = {
        .puerto_actual = inicio,
        .puerto_final  = fin,
        .tabla         = tabla
    };
    pthread_mutex_init(&ctx.lock, NULL);

    for (int i = 0; i < NUM_HILOS; ++i)
        pthread_create(&hilos[i], NULL, trabajador, &ctx);

    for (int i = 0; i < NUM_HILOS; ++i)
        pthread_join(hilos[i], NULL);

    pthread_mutex_destroy(&ctx.lock);
}
