#define _DEFAULT_SOURCE

#include "port_scanner.h"
#include "port_utils.h"

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

// === ESTRUCTURAS ===
typedef struct {
    int puerto_actual;
    int puerto_final;
    pthread_mutex_t lock;
    GHashTable *tabla;
} ContextoEscaneo;

typedef struct {
    int valido;            // 1 si la verificación coincide o se acepta, 0 si no
    char banner[512];      // Banner recibido/payload leído
} ResultadoVerificacion;

// === PROTOTIPOS ===
ResultadoVerificacion verificar_servicio(const char* servicio, int puerto);

// === FUNCIONES AUXILIARES ===

/*
 * puerto_abierto(int puerto)
 * ---------------------------
 *   Intenta conectar TCP en localhost:puerto con un timeout de 1 segundo.
 *   Retorna 1 si connect() tuvo éxito, 0 en caso contrario.
 *
 *   Usamos varios loopbacks en array por si hay routing especial a 127.x.x.x.
 */
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

        // Timeout de 1 segundo para recibir y enviar
        struct timeval tv = { 1, 0 };
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

/*
 * buscar_inode_por_puerto(int puerto)
 * -----------------------------------
 *   Busca en /proc/net/tcp la línea que corresponda al puerto dado
 *   y retorna el inode asociado. Si no lo encuentra, retorna 0.
 *
 *   Esto se usa para, más adelante, recorrer /proc/<pid>/fd y comparar
 *   enlaces simbólicos a "socket:[inode]" para hallar el PID/proceso.
 */
unsigned long buscar_inode_por_puerto(int puerto) {
    FILE* archivo = fopen("/proc/net/tcp", "r");
    if (!archivo) return 0;

    char linea[512];
    fgets(linea, sizeof(linea), archivo); // Saltar cabecera

    char local[64];
    unsigned long inode;
    while (fgets(linea, sizeof(linea), archivo)) {
        int local_port;
        // Ejemplo de línea:
        //   sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode
        //    0: 0100007F:0016 00000000:0000 0A ...
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

/*
 * mostrar_info_proceso_por_inode(unsigned long inode)
 * ---------------------------------------------------
 *   Recorre /proc/<pid>/fd/* y usa readlink para comparar "socket:[inode]".
 *   Si lo encuentra, recupera el ejecutable real (/proc/<pid>/exe) y el usuario
 *   asociado (/proc/<pid> stat -> st_uid). Imprime:
 *     PID, Usuario y Programa.
 *
 *   Si no halla nada, retorna sin imprimir nada.
 */
void mostrar_info_proceso_por_inode(unsigned long inode) {
    DIR* proc = opendir("/proc");
    if (!proc) return;

    struct dirent* entrada;
    while ((entrada = readdir(proc))) {
        // Solo interesan directorios numéricos (PID)
        if (entrada->d_type != DT_DIR) continue;
        int pid = atoi(entrada->d_name);
        if (pid <= 0) continue;

        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/fd", pid);
        DIR* fds = opendir(path);
        if (!fds) continue;

        struct dirent* fd;
        while ((fd = readdir(fds))) {
            // Construir ruta /proc/<pid>/fd/<fdnum>
            char enlace[256], destino[256];
            snprintf(enlace, sizeof(enlace), "%s/%s", path, fd->d_name);

            ssize_t len = readlink(enlace, destino, sizeof(destino) - 1);
            if (len != -1) {
                destino[len] = '\0';
                char buscado[64];
                snprintf(buscado, sizeof(buscado), "socket:[%lu]", inode);
                if (strcmp(destino, buscado) == 0) {
                    // Encontramos el socket; ahora mostramos PID, usuario y ejecutable
                    char exe_path[256], exe[256];
                    snprintf(exe_path, sizeof(exe_path), "/proc/%d/exe", pid);
                    ssize_t exe_len = readlink(exe_path, exe, sizeof(exe) - 1);
                    if (exe_len > 0) exe[exe_len] = '\0';
                    else exe[0] = '\0';

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

/*
 * trabajador(void *arg)
 * ---------------------
 *   Rutina que ejecuta cada hilo trabajador. 
 *   Toma un puerto del ContextoEscaneo y, si está abierto,
 *   busca su servicio en la tabla y ejecuta la lógica de verificación.
 *
 *   - Si el servicio es conocido, invoca verificar_servicio() y
 *     compara. Imprime ✅ o ⚠ según corresponda.
 *   - Si el servicio NO es conocido, imprime “DESCONOCIDO” y busca PID.
 *
 *   Notas:
 *   - Se usa pthread_mutex para distribuir puertos entre hilos.
 *   - Retorna NULL al terminar.
 */
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
                    // Caso esperado: banner coincidente o simple conexión válida (para HTTPS)
                    printf("PUERTO ABIERTO: %d → Servicio: %s ✅ esperado\n", puerto, svc);
                } else {
                    // Comportamiento no coincide: imprimimos banner (si existe) y PID
                    printf("⚠️ PUERTO ABIERTO: %d → Servicio esperado: %s, pero comportamiento NO coincide\n", puerto, svc);
                    if (strlen(verif.banner) > 0) {
                        printf("    ↪ Banner recibido: %s\n", verif.banner);
                    } else {
                        // Explicación: no hubo banner; por ejemplo, netcat no envía nada sin echo
                        printf("    ↪ No se recibió banner del servicio.\n");
                    }
                    unsigned long inode = buscar_inode_por_puerto(puerto);
                    if (inode != 0) {
                        mostrar_info_proceso_por_inode(inode);
                    } else {
                        printf("    ↪ No se pudo determinar el proceso asociado (inode no hallado).\n");
                    }
                }
            } else {
                // Servicio no está en la tabla → marca como sospechoso
                printf("⚠️ PUERTO ABIERTO: %d → DESCONOCIDO (NO en tabla)\n", puerto);
                unsigned long inode = buscar_inode_por_puerto(puerto);
                if (inode != 0) {
                    mostrar_info_proceso_por_inode(inode);
                } else {
                    printf("    ↪ No se pudo determinar el proceso asociado (inode no hallado).\n");
                }
            }
        }
    }
    return NULL;
}

// Puertos adicionales >1024 (sirven para HTTP alternativo, SSH alternativo, FTP alternativo, etc.)
static const int puertos_extra[] = {
    2121,  // FTP alternativo
    2222,  // SSH alternativo
    2525,  // SMTP alternativo
    31337, // Netcat “secret” (sospechoso)
    8080,  // HTTP alternativo
    8000,  // HTTP alternativo
    8443   // HTTPS alternativo
};
static const int total_extra = sizeof(puertos_extra) / sizeof(puertos_extra[0]);

/*
 * escanear_puertos(GHashTable *tabla, int inicio, int fin)
 * -------------------------------------------------------
 *   Crea NUM_HILOS hilos que llamarán a trabajador() para escanear
 *   el rango [inicio..fin].
 *   Además, si ese rango es exactamente 1–1024 (escaneo base), luego
 *   vuelve a llamar recursivamente para cada puerto en puertos_extra[]
 *   pero SOLO en esa llamada base. Así evitamos recursión infinita.
 */
void escanear_puertos(GHashTable *tabla, int inicio, int fin) {
    pthread_t hilos[NUM_HILOS];
    ContextoEscaneo ctx = {
        .puerto_actual = inicio,
        .puerto_final  = fin,
        .tabla         = tabla
    };
    pthread_mutex_init(&ctx.lock, NULL);

    for (int i = 0; i < NUM_HILOS; ++i) {
        pthread_create(&hilos[i], NULL, trabajador, &ctx);
    }

    for (int i = 0; i < NUM_HILOS; ++i) {
        pthread_join(hilos[i], NULL);
    }

    pthread_mutex_destroy(&ctx.lock);

    // Solo al escanear el rango base 1–1024, incluimos los puertos extra
    if (inicio == 1 && fin == 1024) {
        for (int i = 0; i < total_extra; ++i) {
            escanear_puertos(tabla, puertos_extra[i], puertos_extra[i]);
        }
    }
}

/*
 * verificar_servicio(const char* servicio, int puerto)
 * ----------------------------------------------------
 *   Conecta a localhost:puerto y, según el nombre de servicio,
 *   envía la petición adecuada y lee el banner. Retorna:
 *     .valido = 1 si:
 *       - Para HTTP: banner que empiece con "HTTP/".
 *       - Para HTTPS: simplemente que la conexión TCP tenga éxito.
 *       - Para SSH: banner que contenga "SSH-".
 *       - Para SMTP: banner con "220" o "250", o si netcat devuelve algo (len > 0).
 *       - Para POP3: banner con "+OK".
 *       - Para IMAP: banner con "* OK".
 *       - Para FTP: banner con "220".
 *       - Para Telnet: sin leer banner, se acepta la conexión.
 *     .valido = 0 en caso contrario.
 *   Además guarda en .banner el contenido recibido (o "" si no llegó nada).
 */
ResultadoVerificacion verificar_servicio(const char* servicio, int puerto) {
    ResultadoVerificacion resultado = { .valido = 0, .banner = "" };

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return resultado;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(puerto),
        .sin_addr.s_addr = inet_addr("127.0.0.1")
    };

    // Timeout de 1 segundo en recv/send
    struct timeval tv = { 1, 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        close(sockfd);
        return resultado;  // No se pudo conectar
    }

    char buffer[512] = {0};
    ssize_t bytes_recibidos = 0;

    if (strcmp(servicio, "HTTP") == 0) {
        const char *req = "HEAD / HTTP/1.0\r\n\r\n";
        send(sockfd, req, strlen(req), 0);
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_recibidos > 0 && strncmp(buffer, "HTTP/", 5) == 0) {
            resultado.valido = 1;
        }
    }
    else if (strcmp(servicio, "HTTPS") == 0) {
        // Para HTTPS (TLS), no obtenemos texto con HEAD, 
        // pero si la conexión TCP fue exitosa, aceptamos
        resultado.valido = 1;
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        // buffer puede contener datos binarios de TLS o estar vacío
    }
    else if (strcmp(servicio, "SSH") == 0) {
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_recibidos > 0 && strstr(buffer, "SSH-") != NULL) {
            resultado.valido = 1;
        }
    }
    else if (strstr(servicio, "SMTP")) {
        // Recibimos banner inicial (si existe)
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        // Enviamos EHLO para forzar respuesta
        const char* ehlo = "EHLO prueba\r\n";
        send(sockfd, ehlo, strlen(ehlo), 0);
        // Leemos la respuesta
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        // Válido si contiene "220" o "250", o si algo fue recibido
        if ((bytes_recibidos > 0 && (strstr(buffer, "220") || strstr(buffer, "250"))) 
            || bytes_recibidos > 0) {
            resultado.valido = 1;
        }
    }
    else if (strstr(servicio, "POP3")) {
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_recibidos > 0 && strstr(buffer, "+OK") != NULL) {
            resultado.valido = 1;
        }
    }
    else if (strstr(servicio, "IMAP")) {
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_recibidos > 0 && strstr(buffer, "* OK") != NULL) {
            resultado.valido = 1;
        }
    }
    else if (strstr(servicio, "FTP")) {
        bytes_recibidos = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_recibidos > 0 && strstr(buffer, "220") != NULL) {
            resultado.valido = 1;
        }
    }
    else if (strcmp(servicio, "Telnet") == 0) {
        // Telnet no envía banner hasta que se envíen datos,
        // pero si la conexión no falla, lo marcamos como válido
        resultado.valido = 1;
    }
    else {
        // Para cualquier otro servicio común no manejado específicamente,
        // asumimos que la conexión es suficiente para marcarlo válido
        resultado.valido = 1;
    }

    // Guardamos el banner o lo que se haya recibido, si bytes_recibidos > 0
    if (bytes_recibidos > 0) {
        buffer[bytes_recibidos] = '\0';
        strncpy(resultado.banner, buffer, sizeof(resultado.banner) - 1);
    }

    close(sockfd);
    return resultado;
}
