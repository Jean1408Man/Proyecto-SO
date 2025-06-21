#include "port_scanner.h"
#include "models.h"
#include "socket_client.h"
#include "port_utils.h"

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
    }

    // Liberar memoria
    for (int i = 0; i < res.size; i++) {
        free(res.data[i].banner);
        free(res.data[i].user);
        free(res.data[i].process_name);
    }
    free(res.data);

    // --------------------------------------------
    // 2) Procesar puertos de la tabla fuera de rango
    // --------------------------------------------
    GHashTable *tabla = inicializar_tabla_puertos();
    if (!tabla) {
        send_msg("ERROR: No se pudo inicializar la tabla de puertos.\n");
        return EXIT_FAILURE;
    }

    // Obtener todas las claves (puertos) de la tabla
    GList *claves = g_hash_table_get_keys(tabla);
    if (!claves) {
        send_msg("ERROR: La tabla de puertos está vacía.\n");
        g_hash_table_destroy(tabla);
        return EXIT_FAILURE;
    }

    for (GList *l = claves; l != NULL; l = l->next) {
        int puerto_tabla = *(int *)(l->data);
        if (puerto_tabla < start_port || puerto_tabla > end_port) {
            const char *serv = buscar_servicio(tabla, puerto_tabla);
            char aviso[256];
            snprintf(aviso, sizeof(aviso),
                     "\nEscaneando puerto de tabla fuera de rango: %d (%s)\n",
                     puerto_tabla,
                     serv ? serv : "desconocido");
            send_msg(aviso);

            // Escaneo puntual de un solo puerto: [puerto_tabla..puerto_tabla]
            ScanResult resultado_unico = scan_ports(puerto_tabla, puerto_tabla);

            // Imprimir igual que en el rango principal
            for (int j = 0; j < resultado_unico.size; j++) {
                ScanOutput *e = &resultado_unico.data[j];
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
                    } 
                    send_msg(mensaje);

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
            }

            // Liberar memoria del resultado puntual
            for (int j = 0; j < resultado_unico.size; j++) {
                free(resultado_unico.data[j].banner);
                free(resultado_unico.data[j].user);
                free(resultado_unico.data[j].process_name);
            }
            free(resultado_unico.data);
        }
    }

    g_list_free(claves);
    g_hash_table_destroy(tabla);

    // -----------------------------------------------------------
    // 3) Procesar puertos altos **solo si** están después del rango
    // -----------------------------------------------------------
    int puertos_altos[] = { 3306, 5432, 6379, 27017, 9200, 5000, 8888 };
    int n_altos = sizeof(puertos_altos) / sizeof(puertos_altos[0]);

    for (int k = 0; k < n_altos; k++) {
        int p = puertos_altos[k];

        if (p > end_port) {
            char aviso_altos[128];
            snprintf(aviso_altos, sizeof(aviso_altos),
                     "\nEscaneando puerto alto sin servicio asociado: %d\n",
                     p);
            send_msg(aviso_altos);

            ScanResult res_alto = scan_ports(p, p);

            // En este caso siempre caerá en “alerta”:
            for (int m = 0; m < res_alto.size; m++) {
                ScanOutput *e = &res_alto.data[m];
                char mensaje[512] = {0};

                snprintf(mensaje, sizeof(mensaje),
                         "[ALERTA] Puerto %d/tcp abierto. Banner: \"%s\"\n",
                         e->port, e->banner);
                send_msg(mensaje);

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

            // Liberar memoria del resultado puntual
            for (int m = 0; m < res_alto.size; m++) {
                free(res_alto.data[m].banner);
                free(res_alto.data[m].user);
                free(res_alto.data[m].process_name);
            }
            free(res_alto.data);
        }
    }


    return EXIT_SUCCESS;
}
