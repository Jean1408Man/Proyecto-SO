#include "process_monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#define MAX_PROC 4096

// Estructura interna para datos de proceso
// No expuesta fuera de este módulo
typedef struct {
    int     pid;
    unsigned long long utime;
    unsigned long long stime;
    char    cmd[64];
    size_t  mem_kib;
} proc_raw_t;

// Variables globales
typedef struct {
    int count;    // muestras consecutivas sobre umbral
    int alerted;  // ya alertado en este episodio
} spike_state_t;

static pm_config_t   cfg;
static proc_raw_t   *prev        = NULL;
static size_t        prev_len    = 0;
static unsigned long long prev_total = 0;

// Cola circular de alertas
static pm_alert_t    pending[64];
static int           head = 0, tail = 0;

// Estados independientes para CPU y MEM
static spike_state_t *states_cpu = NULL;
static spike_state_t *states_mem = NULL;
static int           required_samples = 0;

// Lee jiffies totales del sistema (/proc/stat)
static unsigned long long read_total_jiffies(void) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return 0;
    unsigned long long user, nice_, sys, idle;
    if (fscanf(f, "cpu %llu %llu %llu %llu", &user, &nice_, &sys, &idle) != 4) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return user + nice_ + sys + idle;
}

int pm_init(const pm_config_t *user_cfg) {
    // Copia configuración y calcula muestras requeridas
    cfg = *user_cfg;
    required_samples = cfg.duration_s * 1000 / cfg.interval_ms;

    // Reserva buffers
    prev        = calloc(MAX_PROC, sizeof(proc_raw_t));
    states_cpu  = calloc(MAX_PROC, sizeof(spike_state_t));
    states_mem  = calloc(MAX_PROC, sizeof(spike_state_t));
    if (!prev || !states_cpu || !states_mem) {
        free(prev);
        free(states_cpu);
        free(states_mem);
        return -1;
    }

    prev_len   = 0;
    prev_total = read_total_jiffies();
    return 0;
}

int pm_sample(void) {
    // 1) Leer delta de jiffies
    unsigned long long curr_total  = read_total_jiffies();
    unsigned long long delta_total = curr_total - prev_total;
    prev_total = curr_total;

    // 2) Capturar snapshot de procesos
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

        // /proc/[pid]/stat
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        if (!(f = fopen(path, "r"))) continue;
        if (fscanf(f,
                   "%*d (%63[^)]) %*c %*d %*d %*d %*d %*d "
                   "%*u %*u %*u %*u %*u %llu %llu",
                   curr[curr_len].cmd, &utime, &stime) < 3) {
            fclose(f);
            continue;
        }
        fclose(f);

        // /proc/[pid]/status → VmRSS
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

    // 3) Detección de picos sostenidos
    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    for (size_t i = 0; i < curr_len; ++i) {
        // CPU % ajustado a núcleos
        float cpu_pct = 0.0f;
        for (size_t j = 0; j < prev_len; ++j) {
            if (curr[i].pid == prev[j].pid) {
                unsigned long long delta_j =
                    (curr[i].utime + curr[i].stime)
                  - (prev[j].utime + prev[j].stime);
                cpu_pct = delta_total
                        ? (float)delta_j * 100.0f * nproc / (float)delta_total
                        : 0.0f;
                break;
            }
        }
        int cpu_spike = cpu_pct > cfg.cpu_threshold;
        int mem_spike = curr[i].mem_kib > cfg.mem_threshold;

        // Estado CPU
        spike_state_t *st_cpu = &states_cpu[i];
        if (cpu_spike) {
            st_cpu->count++;
            if (st_cpu->count >= required_samples && !st_cpu->alerted) {
                pm_alert_t alert;
                alert.type = PM_CPU_SPIKE;
                alert.proc.pid = curr[i].pid;
                strcpy(alert.proc.cmd, curr[i].cmd);
                alert.proc.cpu_percent = cpu_pct;
                alert.proc.mem_kib = curr[i].mem_kib;
                pending[tail] = alert;
                tail = (tail + 1) % 64;
                st_cpu->alerted = 1;
            }
        } else {
            st_cpu->count = 0;
            st_cpu->alerted = 0;
        }

        // Estado MEM
        spike_state_t *st_mem = &states_mem[i];
        if (mem_spike) {
            st_mem->count++;
            if (st_mem->count >= required_samples && !st_mem->alerted) {
                pm_alert_t alert;
                alert.type = PM_MEM_SPIKE;
                alert.proc.pid = curr[i].pid;
                strcpy(alert.proc.cmd, curr[i].cmd);
                alert.proc.cpu_percent = 0.0f;
                alert.proc.mem_kib = curr[i].mem_kib;
                pending[tail] = alert;
                tail = (tail + 1) % 64;
                st_mem->alerted = 1;
            }
        } else {
            st_mem->count = 0;
            st_mem->alerted = 0;
        }
    }

    // 4) Actualizar prev
    free(prev);
    prev     = curr;
    prev_len = curr_len;
    return 0;
}

int pm_get_alert(pm_alert_t *out) {
    if (head == tail) return 0;
    *out = pending[head];
    head = (head + 1) % 64;
    return 1;
}

void pm_shutdown(void) {
    free(prev);
    free(states_cpu);
    free(states_mem);
    prev_len = 0;
    states_cpu = NULL;
    states_mem = NULL;
}
