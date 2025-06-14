#define _XOPEN_SOURCE 700
#include "process_monitor.h"
#include <stdio.h>
#include <unistd.h>   // usleep

int main(void) {
    /* thresholds 70% CPU, 500 MiB RAM, intervalo 1 s */
    pm_config_t cfg = { 70.0f, 512000, 1000 };

    if (pm_init(&cfg) != 0) {
        fprintf(stderr, "Error initializing monitor\n");
        return 1;
    }

    /* 1) Muestra inicial (sin alertas) */
    pm_sample();

    /* 2) Bucle de muestreo */
    for (int iter = 0; iter < 30; ++iter) {
        usleep(cfg.interval_ms * 1000);  // espera 1 s
        pm_sample();

        pm_alert_t a;
        while (pm_get_alert(&a)) {
            fprintf(stderr,
                "[ALERTA] PID %d (%s) %s pico → CPUΔ=%.0f%% | RAM=%zu KiB\n",
                a.proc.pid, a.proc.cmd,
                a.type == PM_CPU_SPIKE ? "CPU" : "MEM",
                a.proc.cpu_percent, a.proc.mem_kib);
        }
    }

    pm_shutdown();
    return 0;
}
