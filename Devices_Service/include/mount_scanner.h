#ifndef MOUNT_SCANNER_H
#define MOUNT_SCANNER_H
#include <stddef.h>

typedef struct {
    char device[64];
    char mountpoint[128];
    char fstype[16];
} MountInfo;

/* Devuelve cuántos llenó y coloca los datos en 'out' (capacidad cap). */
size_t obtener_montajes_usb(MountInfo *out, size_t cap);

#endif
