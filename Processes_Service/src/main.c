#define _XOPEN_SOURCE 700
#include "process_monitor.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h> 

int main(void)
{
    pm_config_t cfg = { 5000.0f, 512000, 1000 };   /* jiffies, KiB, ms */
    pm_init(&cfg);

    for (int k = 0; k < 30; ++k) {                 /* 30 iteraciones */
        pm_sample();

        pm_alert_t a;
        while (pm_get_alert(&a)) {
            fprintf(stderr, "[ALERTA] PID %d (%s) %s pico → CPUΔ=%.0f | RAM=%zu KiB\n",
                    a.proc.pid, a.proc.cmd,
                    a.type == PM_CPU_SPIKE ? "CPU" : "MEM",
                    a.proc.cpu_percent, a.proc.mem_kib);
        }
        struct timespec ts;
        ts.tv_sec = cfg.interval_ms / 1000;
        ts.tv_nsec = (cfg.interval_ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
    }
    pm_shutdown();
    return 0;
}
