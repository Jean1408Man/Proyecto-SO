#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    float  cpu_threshold;
    size_t mem_threshold;
    int    interval_ms;
} pm_config_t;

int  pm_init(const pm_config_t *cfg);
int  pm_sample(void);
int  pm_get_alert(pm_alert_t *out);
void pm_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PROCESS_MONITOR_H */
