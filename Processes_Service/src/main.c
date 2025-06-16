#define _XOPEN_SOURCE 700
#include "process_monitor.h"
#include <stdio.h>
#include <unistd.h>   // usleep
#include <stdlib.h>   // exit

/* Función para leer MemTotal en KiB de /proc/meminfo */
static size_t read_total_mem_kib(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) exit(1);
    char label[32];
    size_t mem_kib = 0;
    if (fscanf(f, "%31s %zu kB", label, &mem_kib) != 2) {
        fclose(f);
        exit(1);
    }
    fclose(f);
    return mem_kib;
}

int main(void) {
    /* 1) Umbral dinámico de RAM al 50% */
    size_t total_kib  = read_total_mem_kib();
    size_t mem_thresh = total_kib * 10 / 100;  // 10% de RAM
    printf("RAM total: %zu KiB (%.1f GiB), umbral (10%%): %zu KiB (%.1f GiB)\n",
           total_kib, total_kib/1024.0/1024.0,
           mem_thresh, mem_thresh/1024.0/1024.0);

    /* 2) Configuración: CPU 70%, RAM 50%, intervalo 1 s, pico sostenido 10 s */
    pm_config_t cfg = {
        70.0f,       /* cpu_threshold % */
        mem_thresh,  /* mem_threshold KiB */
        1000,        /* interval_ms */
        10           /* duration_s */
    };

    if (pm_init(&cfg) != 0) {
        fprintf(stderr, "Error inicializando el monitor\n");
        return 1;
    }

    /* 3) Muestra inicial (sin alertas) */
    pm_sample();

    /* 4) Bucle de muestreo */
    for (int iter = 0; iter < 30; ++iter) /*cambiar cantidad de iteraciones, seteadas en 2 para testear*/{
        usleep(cfg.interval_ms * 1000);
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
