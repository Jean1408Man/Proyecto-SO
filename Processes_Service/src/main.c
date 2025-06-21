#define _XOPEN_SOURCE 700
#define MAX_CMD_LEN 256
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

/* Estructura y funciones para guardar las alertas que seran enviadas a la UI */
#define MAX_MSG_LEN 512
typedef struct {
    char mensaje[MAX_MSG_LEN];  // Aquí va el texto ya generado
} Alert;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <cpu_umbral> <mem_umbral> <duration>\n", argv[0]);
        return 1;
    }

    // Leer argumentos de línea de comandos
    float cpu_umbral = atof(argv[1]);
    float mem_umbral = atof(argv[2]);
    int duration = atoi(argv[3]);

    /* 1) Umbral dinámico de RAM al % recibido como argumento */
    size_t total_kib  = read_total_mem_kib();
    size_t mem_thresh = total_kib * mem_umbral / 100;  // 10% de RAM
    

    /* 2) Configuración: CPU%, RAM%, intervalo?, pico sostenido(duration_s) */
    pm_config_t cfg = {
        cpu_umbral,  /* cpu_threshold % */
        mem_thresh,  /* mem_threshold KiB */
        1000,        /* interval_ms */ 
        duration     /* duration_s */
    };

    /* 3) Inicializacion de la Whitelist */
    WhiteList whitelist;
    whitelist.size = 0;

    const char *ruta = getenv("WHITELIST_FILE");
    if (!ruta) {
        fprintf(stderr, "[ERROR] No se definió la variable WHITELIST_FILE\n");
        exit(1); 
    }

    load_whitelist_from_file(&whitelist, ruta);

    /* 4) Muestra inicial (sin alertas) */
    if (pm_init(&cfg) != 0) {
        fprintf(stderr, "Error inicializando el monitor\n");
        return 1;
    }  

    pm_sample();

    /* 5) Bucle de muestreo */
    for (int iter = 0; iter < 30; ++iter) /*cambiar cantidad de iteraciones, seteadas en 30 para testear*/{
        
        usleep(cfg.interval_ms * 1000);
        pm_sample();

        pm_alert_t a;
        Alert alerta;

        while (pm_get_alert(&a)) {
            
            if(find_in_whitelist(&whitelist, a.proc.cmd))
            {
                fprintf(stderr, "[INFO] PID %d (%s) en la whitelist, ignorando alerta\n",
                        a.proc.pid, a.proc.cmd);
                continue; // Ignorar procesos en la whitelist
            }

            snprintf(alerta.mensaje, MAX_MSG_LEN,
            "[ALERTA] PID %d (%s) %s pico → CPUΔ=%.0f%% | RAM=%zu KiB\n",
            a.proc.pid, a.proc.cmd,
            a.type == PM_CPU_SPIKE ? "CPU" : "MEM",
            a.proc.cpu_percent, a.proc.mem_kib);
                
            /*Esto es para testear en consola*/            
            fprintf(stderr,
                "[ALERTA] PID %d (%s) %s pico → CPUΔ=%.0f%% | RAM=%zu KiB\n",
                a.proc.pid, a.proc.cmd,
                a.type == PM_CPU_SPIKE ? "CPU" : "MEM",
                a.proc.cpu_percent, a.proc.mem_kib);
            
            // Enviar alerta a la UI
            send_msg(alerta.mensaje);
        
        }
    }

    pm_shutdown();

    return 0;
}
