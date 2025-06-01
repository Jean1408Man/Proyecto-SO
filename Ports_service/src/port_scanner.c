// src/port_scanner.c

#include "port_scanner.h"
#include "scanner_utils.h"
#include "port_utils.h"
#include "models.h"

#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <limits.h>
#include <ctype.h>

#define MAX_THREADS 100

static int s_start_port;
static int s_end_port;
static int s_next_port;
static int s_output_index;
static ScanOutput *s_output;    // arreglo pre-alloc de tamaño (end - start + 1)
static GHashTable *s_tabla;     // tabla de puertos/servicios
static pthread_mutex_t s_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Lee de /proc/[pid]/status la línea "Uid:" y devuelve el UID real en *uid_out.
 * Retorna 0 si tuvo éxito, -1 en caso contrario.
 */
static int get_uid_of_pid(pid_t pid, uid_t *uid_out) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[256];
    uid_t uid = (uid_t)-1;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            // Formato: "Uid:\t<real>\t<effective>\t..."
            char *p = line + 4;
            while (*p == '\t' || *p == ' ') p++;
            uid = (uid_t)atoi(p);
            break;
        }
    }
    fclose(f);
    if (uid == (uid_t)-1) return -1;
    *uid_out = uid;
    return 0;
}

/**
 * Dado un UID, devuelve con strdup el nombre de usuario (getpwuid).
 * Si no se encuentra, retorna NULL.
 */
static char *get_username_from_uid(uid_t uid) {
    struct passwd *pwd = getpwuid(uid);
    if (!pwd) return NULL;
    return strdup(pwd->pw_name);
}

/**
 * Para un puerto TCP dado, ejecuta `ss -ltnp sport = :<port>` y parsea la salida
 * para extraer el PID y nombre del proceso que está en LISTEN en ese puerto.
 * Luego obtiene el UID real del PID para averiguar el usuario.
 *
 * - Si todo sale bien, escribe en *pid_out, *user_out (malloc’d), *proc_name_out (malloc’d).
 *   Devuelve 0.
 * - En caso de cualquier fallo, libera lo asignado y devuelve -1.
 */
static int get_process_info(int port, pid_t *pid_out, char **user_out, char **proc_name_out) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "ss -ltnp sport = :%d 2>/dev/null", port);

    FILE *fp = popen(cmd, "r");
    if (!fp) return -1;

    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Buscamos una línea que contenga "LISTEN"
        if (strstr(line, "LISTEN")) {
            // Ejemplo de línea SS:
            // LISTEN  0  128  0.0.0.0:80   0.0.0.0:*   users:(("nginx",pid=1234,fd=4))
            // ó (si hay IPv6):
            // LISTEN  0  100  [::]:443   [::]:*   users:(("apache2",pid=4321,fd=6))

            // 1) Extraer pid=
            char *p_pid = strstr(line, "pid=");
            if (!p_pid) continue;
            p_pid += 4; // avanzar justo después de "pid="
            pid_t pid = (pid_t)atoi(p_pid);
            if (pid <= 0) continue;

            // 2) Extraer process_name: va entre comillas justo antes de ,pid=
            char *p1 = strchr(line, '"');
            if (!p1) {
                // no halló comilla; ponemos "N/A"
                *proc_name_out = strdup("N/A");
            } else {
                char *p2 = strchr(p1 + 1, '"');
                if (!p2) {
                    *proc_name_out = strdup("N/A");
                } else {
                    size_t len = p2 - (p1 + 1);
                    *proc_name_out = malloc(len + 1);
                    strncpy(*proc_name_out, p1 + 1, len);
                    (*proc_name_out)[len] = '\0';
                }
            }

            *pid_out = pid;
            found = 1;
            break;
        }
    }
    pclose(fp);

    if (!found) {
        return -1;
    }

    // 3) Con el PID, obtenemos UID real
    uid_t uid;
    if (get_uid_of_pid(*pid_out, &uid) != 0) {
        *user_out = strdup("N/A");
    } else {
        char *username = get_username_from_uid(uid);
        if (username) {
            *user_out = username;
        } else {
            *user_out = strdup("N/A");
        }
    }
    return 0;
}

/**
 * Función que cada hilo ejecutará:
 *  - Toma puertos atómicamente de s_next_port.
 *  - Intenta conectar, lee banner, busca palabra peligrosa.
 *  - Busca en la tabla y compara con banner esperado.
 *  - Si hay alerta (no está en tabla o banner inesperado), llama a get_process_info.
 *  - Llena un ScanOutput y lo guarda en s_output[] (bajo mutex).
 */
static void *scan_thread(void *arg) {
    (void)arg;
    while (1) {
        int port;
        // ===== 1) Tomar atómicamente el siguiente puerto =====
        pthread_mutex_lock(&s_mutex);
        if (s_next_port > s_end_port) {
            pthread_mutex_unlock(&s_mutex);
            break;
        }
        port = s_next_port;
        s_next_port++;
        pthread_mutex_unlock(&s_mutex);

        // ===== 2) Intentar conectar =====
        int sockfd = connect_to_port(port);
        if (sockfd < 0) {
            // Puerto cerrado o filtrado
            continue;
        }

        // ===== 3) Leer banner =====
        char banner_buf[256] = {0};
        int nbytes = grab_banner(sockfd, banner_buf, sizeof(banner_buf) - 1);
        if (nbytes < 0) {
            nbytes = 0;
            banner_buf[0] = '\0';
        }

        // ===== 4) Buscar palabra peligrosa =====
        const char *found_word = search_dangerous_words(banner_buf, nbytes);

        // ===== 5) Clasificar según la tabla y comparar banner esperado =====
        const char *servicio_esperado = buscar_servicio(s_tabla, port);
        int port_class = servicio_esperado ? 1 : 0;
        int secure = 0;
        if (servicio_esperado) {
            if (strcasestr(banner_buf, servicio_esperado) != NULL) {
                secure = 1;
            }
        }

        // ===== 6) Preparar ScanOutput =====
        ScanOutput entry;
        entry.port           = port;
        entry.classification = port_class;

        const char *texto_banner = (nbytes > 0) ? banner_buf : "<no banner>";
        entry.banner = malloc(strlen(texto_banner) + 1);
        strcpy(entry.banner, texto_banner);

        const char *texto_palabra = (found_word != NULL)
            ? found_word
            : "Sin palabra peligrosa detectada";
        entry.dangerous_word = malloc(strlen(texto_palabra) + 1);
        strcpy(entry.dangerous_word, texto_palabra);

        entry.security_level = secure;

        // Inicialmente no tenemos info de proceso
        entry.pid          = -1;
        entry.user         = strdup("N/A");
        entry.process_name = strdup("N/A");

        // ===== 7) Si hay alerta, recabar info de proceso =====
        if (entry.classification == 0 || entry.security_level == 0) {
            pid_t pid_tmp;
            char *user_tmp = NULL;
            char *procname_tmp = NULL;
            if (get_process_info(port, &pid_tmp, &user_tmp, &procname_tmp) == 0) {
                free(entry.user);
                free(entry.process_name);
                entry.pid = pid_tmp;
                entry.user = user_tmp;
                entry.process_name = procname_tmp;
            } else {
                // Si falla, dejamos pid=-1 y user/process_name="N/A"
                free(user_tmp);
                free(procname_tmp);
            }
        }

        // ===== 8) Guardar en el arreglo compartido =====
        pthread_mutex_lock(&s_mutex);
        s_output[s_output_index] = entry;
        s_output_index++;
        pthread_mutex_unlock(&s_mutex);

        // ===== 9) Cerrar socket =====
        close_socket(sockfd);
    }
    return NULL;
}

/**
 * Escanea puertos TCP en localhost desde start_port hasta end_port (inclusive).
 * Devuelve un ScanResult con todos los ScanOutput de puertos abiertos.
 * El caller debe liberar cada banner, dangerous_word, user, process_name,
 * y finalmente el arreglo data[].
 */
ScanResult scan_ports(int start_port, int end_port) {
    // 1) Validar y ajustar rango
    if (start_port < 1) start_port = 1;
    if (end_port < start_port) end_port = start_port;
    if (end_port > 65535) end_port = 65535;

    s_start_port   = start_port;
    s_end_port     = end_port;
    s_next_port    = start_port;
    s_output_index = 0;

    int total_ports = end_port - start_port + 1;
    s_output = malloc(sizeof(ScanOutput) * total_ports);
    if (!s_output) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // 2) Inicializar la tabla de puertos/servicios
    s_tabla = inicializar_tabla_puertos();

    // 3) Lanzar hilos
    pthread_t threads[MAX_THREADS];
    for (int i = 0; i < MAX_THREADS; i++) {
        int err = pthread_create(&threads[i], NULL, scan_thread, NULL);
        if (err != 0) {
            fprintf(stderr, "Error al crear hilo %d: %s\n", i, strerror(err));
            // Continuamos intentando con los demás hilos
        }
    }

    // 4) Esperar a que todos acaben
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // 5) Destruir la tabla de puertos: liberar claves y GHashTable
    GList *claves = g_hash_table_get_keys(s_tabla);
    for (GList *l = claves; l != NULL; l = l->next) {
        int *p = (int *)l->data;
        g_free(p);
    }
    g_list_free(claves);
    g_hash_table_destroy(s_tabla);

    // 6) Devolver ScanResult
    ScanResult result;
    result.data = s_output;
    result.size = s_output_index;
    return result;
}
