#include "mount_manager.h"
#include "mount_state.h"
#include "mount_scanner.h"
#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static MountState *tabla = NULL;

static void refresh_mounts(void)
{
    MountState *ms, *tmp; HASH_ITER(hh, tabla, ms, tmp) ms->seen = 0;

    MountInfo list[32]; size_t n = obtener_montajes_usb(list, 32);
    for (size_t i = 0; i < n; ++i) {
        HASH_FIND_STR(tabla, list[i].mountpoint, ms);
        if (!ms) {
            ms = calloc(1, sizeof *ms);
            strcpy(ms->mountpoint, list[i].mountpoint);
            strcpy(ms->device, list[i].device);
            strcpy(ms->fstype, list[i].fstype);
            HASH_ADD_STR(tabla, mountpoint, ms);
            printf(">> MONTADO %s (%s)\n", ms->mountpoint, ms->device);
        }
        ms->seen = 1;
    }
    HASH_ITER(hh, tabla, ms, tmp) if (!ms->seen) {
        printf("<< DESMONTADO %s\n", ms->mountpoint);
        free_snapshot(ms->snapshot);
        HASH_DEL(tabla, ms); free(ms);
    }
}

void run_monitor(unsigned intervalo)
{
    while (1) {
        refresh_mounts();
        MountState *ms, *tmp;
        HASH_ITER(hh, tabla, ms, tmp) {
            FileInfo *nuevo = build_snapshot(ms->mountpoint);
            if (ms->snapshot) diff_snapshots(ms->snapshot, nuevo);
            free_snapshot(ms->snapshot);
            ms->snapshot = nuevo;
        }
        sleep(intervalo);
    }
}
