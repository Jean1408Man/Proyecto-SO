#include "process_monitor.h"
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    pm_config_t cfg = {90.0f, 512000, 1000};
    pm_init(&cfg);

    for (int i = 0; i < 50; ++i) {          /* 50 muestras por demo */
        pm_sample();
        pm_alert_t al;
        while (pm_get_alert(&al)) {
            fprintf(stderr,
                    "[ALERTA] PID %d (%s) %s: %.1f%% CPU / %zu KiB RAM\n",
                    al.proc.pid, al.proc.cmd,
                    al.type == PM_CPU_SPIKE ? "CPU" : "MEM",
                    al.proc.cpu_percent, al.proc.mem_kib);
        }
        usleep(cfg.interval_ms * 1000);
    }
    pm_shutdown();
    return 0;
}
