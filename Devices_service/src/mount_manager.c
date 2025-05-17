#include "mount_manager.h"
#include "mount_state.h"
#include "mount_scanner.h"
#include "snapshot.h"
#include "socket_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

static MountState *tabla = NULL;

static void refresh_mounts(void)
{
    MountState *ms, *tmp;
    HASH_ITER(hh, tabla, ms, tmp) ms->seen = 0;

    MountInfo list[32];
    size_t n = obtener_montajes_usb(list, 32);

    for (size_t i = 0; i < n; ++i) {
        HASH_FIND_STR(tabla, list[i].mountpoint, ms);
        if (!ms) {
            ms = calloc(1, sizeof *ms);
            strcpy(ms->mountpoint, list[i].mountpoint);
            strcpy(ms->device, list[i].device);
            strcpy(ms->fstype, list[i].fstype);
            ms->escaneando = 0;
            pthread_mutex_init(&ms->mutex, NULL);
            HASH_ADD_STR(tabla, mountpoint, ms);

            char msg[256];
            snprintf(msg, sizeof(msg), ">> MONTADO %s (%s)\n", ms->mountpoint, ms->device);
            enviar_mensaje(msg);
        }
        ms->seen = 1;
    }

    HASH_ITER(hh, tabla, ms, tmp) {
        if (!ms->seen) {
            char msg[256];
            snprintf(msg, sizeof(msg), "<< DESMONTADO %s\n", ms->mountpoint);
            enviar_mensaje(msg);

            pthread_mutex_destroy(&ms->mutex);
            free_snapshot(ms->snapshot);
            HASH_DEL(tabla, ms);
            free(ms);
        }
    }
}

typedef struct {
    MountState *ms;
} ScanArgs;

void *scan_device_thread(void *arg) {
    ScanArgs *args = (ScanArgs *)arg;
    MountState *ms = args->ms;

    pthread_mutex_lock(&ms->mutex);
    FileInfo *nuevo = build_snapshot(ms->mountpoint);
    if (ms->snapshot) diff_snapshots(ms->snapshot, nuevo);
    free_snapshot(ms->snapshot);
    ms->snapshot = nuevo;
    pthread_mutex_unlock(&ms->mutex);

    ms->escaneando = 0;
    free(args);
    return NULL;
}

void run_monitor(unsigned intervalo)
{
    while (1) {
        refresh_mounts();

        MountState *ms, *tmp;
        HASH_ITER(hh, tabla, ms, tmp) {
            if (ms->escaneando) continue;
            ms->escaneando = 1;

            pthread_t hilo;
            ScanArgs *args = malloc(sizeof *args);
            args->ms = ms;
            printf("🔍 Revisión: %s\n", ms->mountpoint);
            pthread_create(&hilo, NULL, scan_device_thread, args);
            pthread_detach(hilo);
        }

        sleep(intervalo);
    }
}
