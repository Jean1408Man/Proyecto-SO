#include "port_scanner.h"
#include "models.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Uso:
 *   ./escaner [puerto_inicial puerto_final]
 *
 * Si no se pasan argumentos, se usa 1–6000 por defecto.
 */
int main(int argc, char *argv[]) {
    int start_port = 1;
    int end_port   = 6000;

    if (argc == 3) {
        start_port = atoi(argv[1]);
        end_port   = atoi(argv[2]);
        if (start_port <= 0 || end_port <= 0 || end_port < start_port) {
            fprintf(stderr, "Uso: %s [puerto_inicial puerto_final]\n", argv[0]);
            fprintf(stderr, "  Valores deben ser enteros positivos y end_port >= start_port.\n");
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Uso: %s [puerto_inicial puerto_final]\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("Escaneando puertos TCP en localhost del %d al %d...\n", start_port, end_port);
    ScanResult res = scan_ports(start_port, end_port);

    // Imprimir resultados
    for (int i = 0; i < res.size; i++) {
        ScanOutput *e = &res.data[i];
        // Si coincide en tabla y banner esperado
        if (e->classification == 1 && e->security_level == 1) {
            printf("[OK]     Puerto %d/tcp (en tabla) abierto, banner coincide: \"%s\"\n",
                   e->port, e->banner);
        } else {
            // Cualquier otro caso = ALERTA
            if (e->classification == 1) {
                // Estaba en la tabla, pero banner inesperado
                printf("[ALERTA] Puerto %d/tcp (en tabla) abierto, banner inesperado: \"%s\"\n",
                       e->port, e->banner);
            } else {
                // No estaba en la tabla
                printf("[ALERTA] Puerto %d/tcp abierto (no está en tabla). Banner: \"%s\"\n",
                       e->port, e->banner);
            }
            // Mostrar info de proceso si existe
            if (e->pid != -1) {
                printf("         -> PID: %d, Usuario: %s, Proceso: %s\n",
                       e->pid, e->user, e->process_name);
            } else {
                printf("         -> No se pudo determinar proceso asociado.\n");
            }
        }
        // Si hay palabra peligrosa
        if (strcmp(e->dangerous_word, "Sin palabra peligrosa detectada") != 0) {
            printf("         Palabra peligrosa detectada: %s\n", e->dangerous_word);
        }
    }

    // Liberar todo
    for (int i = 0; i < res.size; i++) {
        free(res.data[i].banner);
        free(res.data[i].dangerous_word);
        free(res.data[i].user);
        free(res.data[i].process_name);
    }
    free(res.data);

    return EXIT_SUCCESS;
}
