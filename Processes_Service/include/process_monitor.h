#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H


#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float   cpu_threshold;   /* % CPU para alerta */
    size_t  mem_threshold;   /* KiB de RAM para alerta */
    int     interval_ms;     /* intervalo entre muestras */
    int     duration_s;      /* segundos mínimos para pico sostenido */
} pm_config_t;

typedef struct {
    int     pid;
    char    cmd[64];
    float   cpu_percent;
    size_t  mem_kib;
} pm_sample_t;

typedef enum {
    PM_CPU_SPIKE,
    PM_MEM_SPIKE
} pm_alert_type;

typedef struct {
    pm_alert_type type;
    pm_sample_t   proc;
} pm_alert_t;

typedef struct {
    char processes[100][256];
    size_t size;
} WhiteList;

int  pm_init(const pm_config_t *cfg);
int  pm_sample(void);
int  pm_get_alert(pm_alert_t *out);
void pm_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_MONITOR_H */
