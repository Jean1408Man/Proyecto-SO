#define _XOPEN_SOURCE 700
#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include "file_info.h"

static FileInfo *tabla = NULL;
static const char *base_root;

#define MAX_THREADS 10
pthread_mutex_t contador_mutex = PTHREAD_MUTEX_INITIALIZER;
int hilos_activos = 0;
pthread_mutex_t tabla_mutex = PTHREAD_MUTEX_INITIALIZER;

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

void procesar_archivo(const char *rel_path, const struct stat *st) {
    if (es_ruta_ignorada(rel_path)) return;

    uint8_t h[32]; if (calc_sha(rel_path, h)) return;

    const char *rel = rel_path + strlen(base_root) + 1;
    FileInfo *fi = malloc(sizeof *fi);
    fi->rel_path = strdup(rel);
    memcpy(fi->sha256, h, 32);
    fi->mtime = st->st_mtime;

    pthread_mutex_lock(&tabla_mutex);
    HASH_ADD_KEYPTR(hh, tabla, fi->rel_path, strlen(fi->rel_path), fi);
    pthread_mutex_unlock(&tabla_mutex);
}

void *escanear_directorio_thread(void *arg);

void escanear_directorio(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

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

                char *ruta = strdup(full_path);
                pthread_t hilo;
                pthread_create(&hilo, NULL, escanear_directorio_thread, ruta);
                pthread_detach(hilo);
            } else {
                pthread_mutex_unlock(&contador_mutex);
                escanear_directorio(full_path);
            }
        } else if (S_ISREG(st.st_mode)) {
            procesar_archivo(full_path, &st);
        }
    }
    closedir(dir);
}

void *escanear_directorio_thread(void *arg) {
    char *path = (char *)arg;
    escanear_directorio(path);
    free(path);

    pthread_mutex_lock(&contador_mutex);
    hilos_activos--;
    pthread_mutex_unlock(&contador_mutex);
    return NULL;
}

FileInfo *build_snapshot(const char *root) {
    tabla = NULL;
    base_root = root;
    escanear_directorio(root);

    while (1) {
        pthread_mutex_lock(&contador_mutex);
        int activos = hilos_activos;
        pthread_mutex_unlock(&contador_mutex);
        if (activos == 0) break;
        usleep(100000);
    }
    return tabla;
}

void diff_snapshots(FileInfo *old, FileInfo *nw) {
    FileInfo *cur, *tmp;
    HASH_ITER(hh, nw, cur, tmp) {
        FileInfo *prev; HASH_FIND_STR(old, cur->rel_path, prev);
        if (!prev)
            printf("[+] %s\n", cur->rel_path);
        else if (memcmp(prev->sha256, cur->sha256, 32))
            printf("[*] %s\n", cur->rel_path);
    }
    HASH_ITER(hh, old, cur, tmp) {
        FileInfo *probe;
        HASH_FIND_STR(nw, cur->rel_path, probe);
        if (!probe)
            printf("[-] %s\n", cur->rel_path);
    }
}

void free_snapshot(FileInfo *snap) {
    FileInfo *cur, *tmp;
    HASH_ITER(hh, snap, cur, tmp) {
        HASH_DEL(snap, cur);
        free(cur->rel_path); free(cur);
    }
}
