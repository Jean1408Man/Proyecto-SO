#define _DEFAULT_SOURCE

#include "port_scanner.h"
#include "port_utils.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <dirent.h>

typedef struct {
    int puerto_actual;
    int puerto_final;
    pthread_mutex_t lock;
    GHashTable *tabla;
} ContextoEscaneo;

typedef struct {
    int valido;
    char banner[512];
} ResultadoVerificacion;

static int puerto_abierto(int puerto) {
    const char* loopbacks[] = {
        "127.0.0.1",
        "127.1.1.1",
        "127.1.2.7"
    };
    const int total_loopbacks = sizeof(loopbacks) / sizeof(loopbacks[0]);

    for (int i = 0; i < total_loopbacks; ++i) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) continue;

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(puerto),
            .sin_addr.s_addr = inet_addr(loopbacks[i])
        };

        struct timeval tv = { 0, 100000 }; // 100 ms
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
            close(sockfd);
            return 1;
        }

        close(sockfd);
    }

    return 0;
}


// Busca el inode de un puerto en /proc/net/tcp
unsigned long buscar_inode_por_puerto(int puerto) {
    FILE* archivo = fopen("/proc/net/tcp", "r");
    if (!archivo) return 0;

    char linea[512];
    fgets(linea, sizeof(linea), archivo); // Saltar cabecera

    char local[64];
    unsigned long inode;
    while (fgets(linea, sizeof(linea), archivo)) {
        int local_port;
        sscanf(linea, "%*d: %64[0-9A-Fa-f]:%x %*s %*s %*s %*s %*s %*s %*s %*s %lu",
               local, &local_port, &inode);

        if (local_port == puerto) {
            fclose(archivo);
            return inode;
        }
    }
    fclose(archivo);
    return 0;
}

// Busca qué proceso está usando ese inode
void mostrar_info_proceso_por_inode(unsigned long inode) {
    DIR* proc = opendir("/proc");
    if (!proc) return;

    struct dirent* entrada;
    while ((entrada = readdir(proc))) {
        if (entrada->d_type != DT_DIR) continue;
        int pid = atoi(entrada->d_name);
        if (pid <= 0) continue;

        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/fd", pid);

        DIR* fds = opendir(path);
        if (!fds) continue;

        struct dirent* fd;
        while ((fd = readdir(fds))) {
            char enlace[256], destino[256];
            snprintf(enlace, sizeof(enlace), "%s/%s", path, fd->d_name);

            ssize_t len = readlink(enlace, destino, sizeof(destino) - 1);
            if (len != -1) {
                destino[len] = '\0';
                char buscado[64];
                snprintf(buscado, sizeof(buscado), "socket:[%lu]", inode);
                if (strcmp(destino, buscado) == 0) {
                    // Obtener programa
                    char exe_path[256], exe[256];
                    snprintf(exe_path, sizeof(exe_path), "/proc/%d/exe", pid);
                    ssize_t exe_len = readlink(exe_path, exe, sizeof(exe) - 1);
                    exe[exe_len > 0 ? exe_len : 0] = '\0';

                    // Obtener usuario
                    struct stat st;
                    snprintf(path, sizeof(path), "/proc/%d", pid);
                    if (stat(path, &st) == 0) {
                        struct passwd* pw = getpwuid(st.st_uid);
                        printf("    ↪ PID: %d, Usuario: %s, Programa: %s\n",
                               pid,
                               pw ? pw->pw_name : "desconocido",
                               exe_len > 0 ? exe : "desconocido");
                    }
                    closedir(fds);
                    closedir(proc);
                    return;
                }
            }
        }
        closedir(fds);
    }
    closedir(proc);
}

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
                ResultadoVerificacion verif = verificar_servicio(svc, puerto);
                if (verif.valido) {
                    printf("PUERTO ABIERTO: %d → Servicio: %s ✅ esperado\n", puerto, svc);
                } else {
                    printf("⚠️ PUERTO ABIERTO: %d → Servicio esperado: %s, pero comportamiento NO coincide\n", puerto, svc);
                    if (strlen(verif.banner) > 0)
                        printf("    ↪ Banner recibido: %s\n", verif.banner);
                    unsigned long inode = buscar_inode_por_puerto(puerto);
                    if (inode != 0) {
                        mostrar_info_proceso_por_inode(inode);
                    } else {
                        printf("    ↪ No se pudo determinar el proceso asociado.\n");
                    }
                }
            }

    }
    return NULL;
}

static const int puertos_extra[] = {2121, 2222, 2525, 8080, 8000, 8443};
static const int total_extra = sizeof(puertos_extra) / sizeof(puertos_extra[0]);

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

    if (fin <= 2121) {
        for (int i = 0; i < total_extra; ++i) {
            escanear_puertos(tabla, puertos_extra[i], puertos_extra[i]);
        }
    }
}


ResultadoVerificacion verificar_servicio(const char* servicio, int puerto) {
    ResultadoVerificacion resultado = { .valido = 0 };
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return resultado;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(puerto),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    struct timeval tv = { 0, 200000 }; // 200 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sockfd);
        return resultado;
    }

    char buffer[512] = {0};

    if (strcmp(servicio, "HTTP") == 0 || strcmp(servicio, "HTTPS") == 0) {
        const char *req = "HEAD / HTTP/1.0\r\n\r\n";
        send(sockfd, req, strlen(req), 0);
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        resultado.valido = strstr(buffer, "HTTP") != NULL;
    } else if (strcmp(servicio, "SSH") == 0) {
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        resultado.valido = strstr(buffer, "SSH-") != NULL;
    } else if (strcmp(servicio, "SMTP") == 0 || strcmp(servicio, "SMTPS") == 0 || strcmp(servicio, "SMTP (STARTTLS)") == 0) {
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        const char* ehlo = "EHLO prueba\r\n";
        send(sockfd, ehlo, strlen(ehlo), 0);
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        resultado.valido = strstr(buffer, "220") != NULL || strstr(buffer, "250") != NULL;
    } else if (strcmp(servicio, "POP3") == 0 || strcmp(servicio, "POP3S") == 0) {
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        resultado.valido = strstr(buffer, "+OK") != NULL;
    } else if (strcmp(servicio, "IMAP") == 0 || strcmp(servicio, "IMAPS") == 0) {
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        resultado.valido = strstr(buffer, "* OK") != NULL;
    } else if (strcmp(servicio, "FTP") == 0 || strcmp(servicio, "FTP-DATA") == 0) {
        recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        resultado.valido = strstr(buffer, "220") != NULL;
    } else if (strcmp(servicio, "Telnet") == 0) {
        resultado.valido = 1;
    } else {
        resultado.valido = 1;
    }

    close(sockfd);
    strncpy(resultado.banner, buffer, sizeof(resultado.banner) - 1);
    return resultado;
}


