#define _GNU_SOURCE         /* getline() */
#include "mount_scanner.h"
#include <stdio.h>
#include <string.h>

static int es_usb(const char *dev, const char *mnt)
{
    return strstr(dev, "/dev/sd") &&
           (strstr(mnt, "/media") || strstr(mnt, "/run/media") || strstr(mnt, "/mnt"));
}

size_t obtener_montajes_usb(MountInfo *out, size_t cap)
{
    FILE *fp = fopen("/proc/mounts", "r");
    if (!fp) return 0;

    char *line = NULL; size_t len = 0;
    size_t n = 0;
    while (getline(&line, &len, fp) != -1 && n < cap) {
        char dev[64], mnt[128], fs[16];
        if (sscanf(line, "%63s %127s %15s", dev, mnt, fs) != 3) continue;
        if (es_usb(dev, mnt)) {
            strcpy(out[n].device, dev);
            strcpy(out[n].mountpoint, mnt);
            strcpy(out[n].fstype, fs);
            ++n;
        }
    }
    free(line);
    fclose(fp);
    return n;
}
