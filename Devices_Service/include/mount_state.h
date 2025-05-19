#ifndef MOUNT_STATE_H
#define MOUNT_STATE_H

#include "file_info.h"   /* usa FileInfo */
#include "uthash.h"
#include <pthread.h>

/* --- Estado de un montaje --- */
typedef struct mount_state {
    char mountpoint[128];   /* clave */
    char device[64];
    char fstype[16];
    FileInfo *snapshot;     /* tabla de FileInfo */
    int  seen;              /* flag interno */
    int  escaneando;        /* para evitar hilos duplicados */
    pthread_mutex_t mutex;  /* para proteger acceso concurrente al snapshot */
    UT_hash_handle hh;      /* enlace uthash */
} MountState;

#endif /* MOUNT_STATE_H */
