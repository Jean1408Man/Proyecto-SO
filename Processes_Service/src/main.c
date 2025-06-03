#define _XOPEN_SOURCE 700
#include "process_monitor.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>

/* --- Función para leer MemTotal en KiB de /proc/meminfo --- */
static size_t read_total_mem_kib(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    char label[32];
    size_t mem_kib = 0;
    if (fscanf(f, "%31s %zu kB", label, &mem_kib) != 2) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return mem_kib;
}

int main(void)
{
    /* Leer RAM total y calcular umbral al 50 % */
    size_t total_kib   = read_total_mem_kib();
    float  mem_percent = 50.0f;                          // umbral al 50%
    size_t threshold   = total_kib * mem_percent / 100;  // KiB

    /* Configuración: CPU 1%, RAM dinámico, intervalo 1s */
    pm_config_t cfg = { 
        70.0f,          /* cpu_threshold % */
        threshold,     /* mem_threshold KiB */
        1000           /* interval_ms */
    };

    //Eliminar comentarios para depurar memoria total y umbral
    // printf("RAM total: %zu KiB, umbral RAM (50%%): %zu KiB\n",
    //        total_kib, threshold);

    pm_init(&cfg);

    for (int k = 0; k < 30; ++k) {
        pm_sample();

        pm_alert_t a;
        while (pm_get_alert(&a)) {
            fprintf(stderr,
                    "[ALERTA] PID %d (%s) %s pico → CPUΔ=%.0f%% | RAM=%zu KiB\n",
                    a.proc.pid, a.proc.cmd,
                    a.type == PM_CPU_SPIKE ? "CPU" : "MEM",
                    a.proc.cpu_percent, a.proc.mem_kib);
        }
        struct timespec ts = {
            .tv_sec  = cfg.interval_ms / 1000,
            .tv_nsec = (cfg.interval_ms % 1000) * 1000000L
        };
        nanosleep(&ts, NULL);
    }

    pm_shutdown();
    return 0;
}
