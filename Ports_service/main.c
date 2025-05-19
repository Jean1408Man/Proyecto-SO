#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <glib.h>

#define MAX_PUERTOS 1024
#define NUM_HILOS 10

typedef struct {
    int puerto_actual;
    int puerto_final;
    pthread_mutex_t lock;
    GHashTable* tabla;
} ContextoEscaneo;

// Detecta si un puerto está abierto en localhost
int puerto_abierto(int puerto) {
    int sockfd;
    struct sockaddr_in direccion;
    struct timeval timeout = {0, 100000}; // 100 ms

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 0;

    direccion.sin_family = AF_INET;
    direccion.sin_port = htons(puerto);
    direccion.sin_addr.s_addr = inet_addr("127.0.0.1");

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    int resultado = connect(sockfd, (struct sockaddr*)&direccion, sizeof(direccion));
    close(sockfd);
    return resultado == 0;
}

// Busca el nombre del servicio en la tabla
const char* buscar_servicio(GHashTable* tabla, int puerto) {
    gpointer valor = g_hash_table_lookup(tabla, &puerto);
    return valor ? (const char*)valor : NULL;
}

// Función que ejecuta cada hilo
void* trabajador(void* arg) {
    ContextoEscaneo* ctx = (ContextoEscaneo*)arg;

    while (1) {
        int puerto;

        // Obtener siguiente puerto de forma segura
        pthread_mutex_lock(&ctx->lock);
        if (ctx->puerto_actual > ctx->puerto_final) {
            pthread_mutex_unlock(&ctx->lock);
            break; // no hay más puertos
        }
        puerto = ctx->puerto_actual++;
        pthread_mutex_unlock(&ctx->lock);

        if (puerto_abierto(puerto)) {
            const char* servicio = buscar_servicio(ctx->tabla, puerto);
            if (servicio) {
                printf("PUERTO ABIERTO: %d → Servicio: %s\n", puerto, servicio);
            } else {
                printf("⚠️ PUERTO ABIERTO: %d → Servicio: DESCONOCIDO (SOSPECHOSO)\n", puerto);
            }
        }
    }

    return NULL;
}

// Inicializa la tabla hash con puertos comunes
GHashTable* inicializar_tabla_puertos() {
    GHashTable* tabla = g_hash_table_new(g_int_hash, g_int_equal);
    static int puertos[] = {
        20, 21, 22, 23, 25, 53, 67, 68, 69, 70, 79, 80,
        110, 111, 123, 135, 137, 138, 139, 143, 161, 162,
        179, 194, 389, 443, 445, 465, 514, 515, 587, 631,
        636, 993, 995, 1024
    };
    const char* servicios[] = {
        "FTP-DATA", "FTP", "SSH", "Telnet", "SMTP", "DNS", "DHCP Server", "DHCP Client", "TFTP",
        "Gopher", "Finger", "HTTP", "POP3", "RPCbind", "NTP", "MS RPC", "NetBIOS NS", "NetBIOS DGM",
        "NetBIOS SSN", "IMAP", "SNMP", "SNMP Trap", "BGP", "IRC", "LDAP", "HTTPS", "SMB",
        "SMTPS", "Syslog", "LPD/LPR", "SMTP (STARTTLS)", "IPP", "LDAPS", "IMAPS", "POP3S"
    };

    int n = sizeof(puertos) / sizeof(int);
    for (int i = 0; i < n; ++i) {
        int* clave = g_new(int, 1);
        *clave = puertos[i];
        g_hash_table_insert(tabla, clave, (gpointer)servicios[i]);
    }
    return tabla;
}

int main() {
    GHashTable* tabla = inicializar_tabla_puertos();
    pthread_t hilos[NUM_HILOS];
    ContextoEscaneo ctx = {
        .puerto_actual = 1,
        .puerto_final = 1024,
        .tabla = tabla
    };
    pthread_mutex_init(&ctx.lock, NULL);

    printf("🔍 Escaneando puertos 1–1024 con %d hilos...\n", NUM_HILOS);

    for (int i = 0; i < NUM_HILOS; ++i) {
        pthread_create(&hilos[i], NULL, trabajador, &ctx);
    }
    for (int i = 0; i < NUM_HILOS; ++i) {
        pthread_join(hilos[i], NULL);
    }

    pthread_mutex_destroy(&ctx.lock);
    g_hash_table_destroy(tabla);
    return 0;
}
