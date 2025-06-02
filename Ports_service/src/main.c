#include "port_scanner.h"
#include "models.h"
#include "socket_client.h"

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
            send_msg("ERROR: Uso: ./escaner [puerto_inicial puerto_final]\n");
            return EXIT_FAILURE;
        }
    } else if (argc != 1) {
        send_msg("ERROR: Uso: ./escaner [puerto_inicial puerto_final]\n");
        return EXIT_FAILURE;
    }

    // Informar inicio de escaneo
    {
        char mensaje[128];
        snprintf(mensaje, sizeof(mensaje),
                 "Escaneando puertos TCP en localhost del %d al %d...\n",
                 start_port, end_port);
        send_msg(mensaje);
    }

    ScanResult res = scan_ports(start_port, end_port);

    // Por cada puerto abierto, formamos un mensaje y lo enviamos
    for (int i = 0; i < res.size; i++) {
        ScanOutput *e = &res.data[i];
        char mensaje[512] = {0};

        if (e->classification == 1 && e->security_level == 1) {
            // Caso [OK]
            snprintf(mensaje, sizeof(mensaje),
                     "[OK]     Puerto %d/tcp abierto con servicio comun asociado, banner coincide: \"%s\"\n",
                     e->port, e->banner);
            send_msg(mensaje);
        } else {
            // Caso [ALERTA]
            if (e->classification == 1) {
                // Estaba en la tabla, pero banner inesperado
                snprintf(mensaje, sizeof(mensaje),
                         "[ALERTA] Puerto %d/tcp abierto con servicio comun asociado, banner inesperado: \"%s\"\n",
                         e->port, e->banner);
            } else {
                // No estaba en la tabla
                snprintf(mensaje, sizeof(mensaje),
                         "[ALERTA] Puerto %d/tcp abierto. Banner: \"%s\"\n",
                         e->port, e->banner);
            }
            send_msg(mensaje);

            // Información de proceso asociado (si existe)
            if (e->pid != -1) {
                snprintf(mensaje, sizeof(mensaje),
                         "         -> PID: %d, Usuario: %s, Proceso: %s\n",
                         e->pid, e->user, e->process_name);
            } else {
                snprintf(mensaje, sizeof(mensaje),
                         "         -> No se pudo determinar proceso asociado.\n");
            }
            send_msg(mensaje);
        }

        if (strcmp(e->dangerous_word, "Sin palabra peligrosa detectada") != 0) {
            snprintf(mensaje, sizeof(mensaje),
                     "         Palabra peligrosa detectada: %s\n",
                     e->dangerous_word);
            send_msg(mensaje);
        }
    }

    // Liberar memoria
    for (int i = 0; i < res.size; i++) {
        free(res.data[i].banner);
        free(res.data[i].dangerous_word);
        free(res.data[i].user);
        free(res.data[i].process_name);
    }
    free(res.data);

    return EXIT_SUCCESS;
}
