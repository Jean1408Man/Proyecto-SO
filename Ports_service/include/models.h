#ifndef MODELS_H
#define MODELS_H

#include <sys/types.h>

/**
 * Para cada puerto abierto, además de banner y nivel de seguridad,
 * guardamos:
 *  - pid: PID del proceso que tiene ese puerto en LISTEN (o -1 si no se pudo obtener).
 *  - user: nombre de usuario (string) que levantó el proceso (o "N/A").
 *  - process_name: nombre del ejecutable (o "N/A").
 */
typedef struct {
    int port;               // número de puerto
    int classification;     // 0 = no está en la tabla, 1 = sí está
    char *banner;           // texto del banner o "<no banner>"
    int security_level;     // 0 = banner inesperado / 1 = coincide

    pid_t pid;              // PID del proceso que escucha en ese puerto (LISTEN). -1 si no se halló.
    char *user;             // usuario que levantó ese proceso o "N/A"
    char *process_name;     // nombre del ejecutable o "N/A"
} ScanOutput;

typedef struct {
    ScanOutput *data;       // arreglo dinámico de ScanOutput
    int size;               // cuántas entradas válidas hay en data[0..size-1]
} ScanResult;

#endif // MODELS_H
