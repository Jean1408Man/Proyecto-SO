#define _GNU_SOURCE         /* getline() */
#include "mount_scanner.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int es_usb(const char *dev, const char *mnt, const char *fs)
{
    // Dispositivo físico (USB real)
    if (strstr(dev, "/dev/sd") && 
        (strstr(mnt, "/media") || strstr(mnt, "/run/media") || strstr(mnt, "/mnt"))) {
        return 1;
    }

    // Dispositivo simulado (tmpfs en /mnt)
    if (strcmp(fs, "tmpfs") == 0 && strstr(mnt, "/mnt/usb")) {
        return 1;
    }

    return 0;
}

size_t obtener_montajes_usb(MountInfo *out, size_t cap)
{
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) return 0;

    char *line = NULL;
    size_t len = 0, n = 0;

    while (getline(&line, &len, fp) != -1 && n < cap) {
        char dev[64], mnt[128], fs[16];
        if (sscanf(line, "%63s %127s %15s", dev, mnt, fs) != 3) continue;

        if (es_usb(dev, mnt, fs)) {
            strncpy(out[n].device, dev, sizeof(out[n].device) - 1);
            strncpy(out[n].mountpoint, mnt, sizeof(out[n].mountpoint) - 1);
            strncpy(out[n].fstype, fs, sizeof(out[n].fstype) - 1);
            ++n;
        }
    }

    free(line);
    fclose(fp);
    return n;
}
