#include "process_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_PROC 4096

typedef struct {
    int     pid;
    unsigned long long utime;
    unsigned long long stime;
    char    cmd[64];
    size_t  mem_kib;
} proc_raw_t;

static pm_config_t cfg;
static proc_raw_t *prev = NULL;
static size_t prev_len = 0;
static unsigned long long prev_total = 0;
static pm_alert_t pending[64];
static int head = 0, tail = 0;

/* Lee jiffies totales del sistema */
static unsigned long long read_total_jiffies(void)
{
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;
    unsigned long long u,n,s,i;
    if (fscanf(f, "cpu %llu %llu %llu %llu", &u, &n, &s, &i) != 4) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return u + n + s + i;
}

int pm_init(const pm_config_t *user_cfg)
{
    cfg = *user_cfg;
    prev = calloc(MAX_PROC, sizeof(proc_raw_t));
    if (!prev) return -1;
    prev_len   = 0;
    prev_total = read_total_jiffies();
    return 0;
}

int pm_sample(void)
{
    /*Leer jiffies globales y calcular delta */
    unsigned long long curr_total = read_total_jiffies();
    unsigned long long delta_total = curr_total - prev_total;
    prev_total = curr_total;

    /*Tomar snapshot de procesos */
    proc_raw_t *curr = calloc(MAX_PROC, sizeof(proc_raw_t));
    if (!curr) return -1;
    size_t curr_len = 0;

    DIR *d = opendir("/proc");
    if (!d) { free(curr); return -1; }

    struct dirent *de;
    while ((de = readdir(d)) && curr_len < MAX_PROC) {
        int pid = atoi(de->d_name);
        if (pid <= 0) continue;

        char path[64], buf[256];
        FILE *f;
        unsigned long long utime, stime;

        /* /proc/[pid]/stat */
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        if (!(f = fopen(path, "r"))) continue;
        if (fscanf(f,
                   "%*d (%63[^)]) %*c %*d %*d %*d %*d %*d "
                   "%*u %*u %*u %*u %*u %llu %llu",
                   curr[curr_len].cmd, &utime, &stime) != 3) {
            fclose(f);
            continue;
        }
        fclose(f);

        /* /proc/[pid]/status → VmRSS */
        curr[curr_len].mem_kib = 0;
        snprintf(path, sizeof(path), "/proc/%d/status", pid);
        if ((f = fopen(path, "r")) != NULL) {
            while (fgets(buf, sizeof(buf), f)) {
                if (sscanf(buf, "VmRSS: %zu", &curr[curr_len].mem_kib) == 1)
                    break;
            }
            fclose(f);
        }

        curr[curr_len].pid   = pid;
        curr[curr_len].utime = utime;
        curr[curr_len].stime = stime;
        curr_len++;
    }
    closedir(d);

    /*Detectar picos comparando curr vs prev */
    for (size_t i = 0; i < curr_len; ++i) {
        /* CPU % */
        for (size_t j = 0; j < prev_len; ++j) {
            if (curr[i].pid == prev[j].pid) {
                unsigned long long delta_j =
                    (curr[i].utime + curr[i].stime)
                  - (prev[j].utime + prev[j].stime);
                float cpu_pct = delta_total
                              ? (float)delta_j * 100.0f / (float)delta_total
                              : 0.0f;
                if (cpu_pct > cfg.cpu_threshold) {
                    pending[tail].type             = PM_CPU_SPIKE;
                    pending[tail].proc.pid         = curr[i].pid;
                    strcpy(pending[tail].proc.cmd, curr[i].cmd);
                    pending[tail].proc.cpu_percent = cpu_pct;
                    pending[tail].proc.mem_kib     = curr[i].mem_kib;
                    tail = (tail + 1) % 64;
                }
                break;
            }
        }
        /* Memoria */
        if (curr[i].mem_kib > cfg.mem_threshold) {
            pending[tail].type             = PM_MEM_SPIKE;
            pending[tail].proc.pid         = curr[i].pid;
            strcpy(pending[tail].proc.cmd, curr[i].cmd);
            pending[tail].proc.cpu_percent = 0.0f;
            pending[tail].proc.mem_kib     = curr[i].mem_kib;
            tail = (tail + 1) % 64;
        }
    }

    /*Actualizar prev */
    free(prev);
    prev     = curr;
    prev_len = curr_len;
    return 0;
}

int pm_get_alert(pm_alert_t *out)
{
    if (head == tail) return 0;
    *out = pending[head];
    head = (head + 1) % 64;
    return 1;
}

void pm_shutdown(void)
{
    free(prev);
    prev = NULL;
    prev_len = 0;
}
