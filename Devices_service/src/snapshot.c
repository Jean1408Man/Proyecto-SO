#define _XOPEN_SOURCE 700
#include "snapshot.h"
#include "socket_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "file_info.h"

static const char *base_root;

#define MAX_THREADS 10
#define MAX_ARCHIVOS 50000
#define MAX_TAMANO_HASH (1 * 1024 * 1024) // 1MB

pthread_mutex_t contador_mutex = PTHREAD_MUTEX_INITIALIZER;
int hilos_activos = 0;
pthread_mutex_t tabla_mutex = PTHREAD_MUTEX_INITIALIZER;
static int contador_archivos = 0;

static int es_ruta_ignorada(const char *rel_path) {
    return strstr(rel_path, "/.Trash") ||
           strstr(rel_path, "/lost+found") ||
           strstr(rel_path, "/.DS_Store") ||
           strstr(rel_path, "/.cache") ||
           strstr(rel_path, "/.Trash-1000");
}

static int calc_sha(const char *p, uint8_t h[32]) {
    FILE *f = fopen(p, "rb"); if (!f) return -1;
    SHA256_CTX c; SHA256_Init(&c);
    uint8_t buf[4096]; size_t r;
    while ((r = fread(buf, 1, sizeof buf, f))) SHA256_Update(&c, buf, r);
    fclose(f); SHA256_Final(h, &c); return 0;
}

void procesar_archivo(const char *rel_path, const struct stat *st, FileInfo **tabla) {
    if (es_ruta_ignorada(rel_path)) return;

    pthread_mutex_lock(&contador_mutex);
    if (++contador_archivos > MAX_ARCHIVOS) {
        pthread_mutex_unlock(&contador_mutex);
        return;
    }
    pthread_mutex_unlock(&contador_mutex);

    printf("📄 Archivo: %s (%ld bytes)\n", rel_path, st->st_size);

    uint8_t h[32] = {0};
    if (st->st_size <= MAX_TAMANO_HASH) {
        calc_sha(rel_path, h);
    }

    const char *rel = rel_path + strlen(base_root) + 1;
    FileInfo *fi = malloc(sizeof *fi);
    fi->rel_path = strdup(rel);
    memcpy(fi->sha256, h, 32);
    fi->mtime = st->st_mtime;
    fi->size = st->st_size;
    fi->mode = st->st_mode;
    fi->uid = st->st_uid;
    fi->gid = st->st_gid;

    pthread_mutex_lock(&tabla_mutex);
    HASH_ADD_KEYPTR(hh, *tabla, fi->rel_path, strlen(fi->rel_path), fi);
    pthread_mutex_unlock(&tabla_mutex);
}

void *escanear_directorio_thread(void *arg);

typedef struct {
    const char *path;
    FileInfo **tabla;
} ScanArgs;

void escanear_directorio(const char *path, FileInfo **tabla) {
    DIR *dir = opendir(path);
    if (!dir) return;

    printf("📁 Explorando: %s\n", path);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            if (es_ruta_ignorada(full_path)) continue;

            pthread_mutex_lock(&contador_mutex);
            if (hilos_activos < MAX_THREADS) {
                hilos_activos++;
                pthread_mutex_unlock(&contador_mutex);

                ScanArgs *args = malloc(sizeof(ScanArgs));
                args->path = strdup(full_path);
                args->tabla = tabla;

                pthread_t hilo;
                pthread_create(&hilo, NULL, escanear_directorio_thread, args);
                pthread_detach(hilo);
            } else {
                pthread_mutex_unlock(&contador_mutex);
                escanear_directorio(full_path, tabla);
            }
        } else if (S_ISREG(st.st_mode)) {
            procesar_archivo(full_path, &st, tabla);
        }
    }
    closedir(dir);
}

void *escanear_directorio_thread(void *arg) {
    ScanArgs *args = (ScanArgs *)arg;
    escanear_directorio(args->path, args->tabla);
    free((void *)args->path);
    free(args);

    pthread_mutex_lock(&contador_mutex);
    hilos_activos--;
    pthread_mutex_unlock(&contador_mutex);
    return NULL;
}

FileInfo *build_snapshot(const char *root) {
    printf("🧪 Entrando a build_snapshot: %s\n", root);
    FileInfo *tabla_local = NULL;
    base_root = root;
    contador_archivos = 0;

    escanear_directorio(root, &tabla_local);

    while (1) {
        pthread_mutex_lock(&contador_mutex);
        int activos = hilos_activos;
        pthread_mutex_unlock(&contador_mutex);
        if (activos == 0) break;
        usleep(100000);
    }

    return tabla_local;
}

void diff_snapshots(FileInfo *old, FileInfo *nw, int umbral_porcentaje) {
    printf("Analizando diferencias");
    FileInfo *cur, *tmp;
    char mensaje[512];
    int total_old = 0;
    int cambios_detectados = 0;

    HASH_ITER(hh, nw, cur, tmp) {
        FileInfo *prev; HASH_FIND_STR(old, cur->rel_path, prev);
        if (!prev) {
            snprintf(mensaje, sizeof(mensaje), "[+] %s\n", cur->rel_path);
            enviar_mensaje(mensaje);
            cambios_detectados++;
        } else {
            int tiene_hash = (cur->sha256[0] != 0 || prev->sha256[0] != 0);
            if (tiene_hash && memcmp(prev->sha256, cur->sha256, 32)) {
                snprintf(mensaje, sizeof(mensaje), "[*] %s\n", cur->rel_path);
                enviar_mensaje(mensaje);
                cambios_detectados++;
            }

            if (cur->size > prev->size * 100 && cur->size > 100 * 1024 * 1024) {
                snprintf(mensaje, sizeof(mensaje), "[!] CRECIMIENTO SOSPECHOSO: %s (%ld → %ld bytes)\n", cur->rel_path, prev->size, cur->size);
                enviar_mensaje(mensaje);
            }

            if (cur->mode != prev->mode) {
                snprintf(mensaje, sizeof(mensaje), "[!] CAMBIO DE PERMISOS: %s (modo %o → %o)\n", cur->rel_path, prev->mode, cur->mode);
                enviar_mensaje(mensaje);
            }

            const char *ext_ant = strrchr(prev->rel_path, '.');
            const char *ext_nue = strrchr(cur->rel_path, '.');
            if (ext_ant && ext_nue && strcmp(ext_ant, ext_nue)) {
                snprintf(mensaje, sizeof(mensaje), "[!] CAMBIO DE EXTENSIÓN: %s (de %s a %s)\n", cur->rel_path, ext_ant, ext_nue);
                enviar_mensaje(mensaje);
            }

            if (cur->uid != prev->uid) {
                snprintf(mensaje, sizeof(mensaje), "[!] CAMBIO DE DUEÑO (UID): %s (%d → %d)\n", cur->rel_path, prev->uid, cur->uid);
                enviar_mensaje(mensaje);
            }

            if (cur->gid != prev->gid) {
                snprintf(mensaje, sizeof(mensaje), "[!] CAMBIO DE GRUPO (GID): %s (%d → %d)\n", cur->rel_path, prev->gid, cur->gid);
                enviar_mensaje(mensaje);
            }

            if (cur->mtime != prev->mtime) {
                snprintf(mensaje, sizeof(mensaje), "[!] MODIFICACIÓN DE TIMESTAMP: %s\n", cur->rel_path);
                enviar_mensaje(mensaje);
            }
        }
    }

    HASH_ITER(hh, old, cur, tmp) {
        total_old++;
        FileInfo *probe;
        HASH_FIND_STR(nw, cur->rel_path, probe);
        if (!probe) {
            snprintf(mensaje, sizeof(mensaje), "[-] %s\n", cur->rel_path);
            enviar_mensaje(mensaje);
            cambios_detectados++;
        }
    }

    FileInfo *a, *b;
    for (a = nw; a != NULL; a = a->hh.next) {
        for (b = a->hh.next; b != NULL; b = b->hh.next) {
            if (memcmp(a->sha256, b->sha256, 32) == 0 &&
                strcmp(a->rel_path, b->rel_path) != 0 &&
                (a->sha256[0] != 0 || b->sha256[0] != 0)) {
                snprintf(mensaje, sizeof(mensaje), "[!] ARCHIVOS DUPLICADOS: %s y %s\n", a->rel_path, b->rel_path);
                enviar_mensaje(mensaje);
            }
        }
    }

    if (total_old > 0) {
        int porcentaje = (cambios_detectados * 100) / total_old;
        if (porcentaje >= umbral_porcentaje) {
            snprintf(mensaje, sizeof(mensaje), "[!!] ALERTA: %d%% de los archivos han cambiado (umbral: %d%%)\n", porcentaje, umbral_porcentaje);
            enviar_mensaje(mensaje);
        }
    }
}

void free_snapshot(FileInfo *snap) {
    FileInfo *cur, *tmp;
    HASH_ITER(hh, snap, cur, tmp) {
        HASH_DEL(snap, cur);
        free(cur->rel_path); free(cur);
    }
}
