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

#endif /* PROCESS_MONITOR_H */#include <stddef.h>
#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct {
    int     pid;
    char    cmd[64];
    float   cpu_percent;   /* % CPU desde la última muestra */
    size_t  mem_kib;       /* memoria residente en KiB */
} pm_sample_t;

typedef enum { PM_CPU_SPIKE, PM_MEM_SPIKE } pm_alert_type;

typedef struct {
    pm_alert_type type;
    pm_sample_t   proc;
} pm_alert_t;

/* Configuración rápida — valores hard-code que luego serán leídos de conf */
typedef struct {
    float  cpu_threshold;   /* p.ej. 90.0  */
    size_t mem_threshold;   /* p.ej. 512000 KiB (500 MiB) */
    int    interval_ms;     /* 1000 por defecto */
} pm_config_t;

/* API mínima */
int  pm_init(const pm_config_t *cfg);
int  pm_sample(void);                    /* toma una muestra global */
int  pm_get_alert(pm_alert_t *out);      /* retorna 1 si hay alerta pendiente */
void pm_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif /* PROCESS_MONITOR_H */
// API mínima para el monitor de procesos
#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

// Declaraciones de funciones para el monitor de procesos

#endif // PROCESS_MONITOR_H
#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <stddef.h>      
#include <stdint.h>      

#ifdef __cplusplus
extern "C" {
#endif
/* ← aquí definimos size_t    */
#include <stdint.h>      /* (opcional, para tipos std) */

typedef struct {
    int     pid;
    char    cmd[64];
    float   cpu_percent;
    size_t  mem_kib;
} pm_sample_t;

typedef enum { PM_CPU_SPIKE, PM_MEM_SPIKE } pm_alert_type;

typedef struct {
    pm_alert_type type;
    pm_sample_t   proc;
} pm_alert_t;

typedef struct {
    float  cpu_threshold;
    size_t mem_threshold;
    int    interval_ms;
} pm_config_t;

/* API mínima */
int  pm_init(const pm_config_t *cfg);
int  pm_sample(void);
int  pm_get_alert(pm_alert_t *out);
void pm_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif /* PROCESS_MONITOR_H */
