#ifndef PORT_SCANNER_H
#define PORT_SCANNER_H

#include <glib.h>

/** Número de hilos que usarán el pool */
#define NUM_HILOS 10

/**
 * Escanea de forma concurrente (pool de hilos) los puertos
 * en el rango [inicio, fin] sobre 127.0.0.1. Por cada puerto
 * abierto imprime en stdout:
 *   - si tiene servicio conocido, lo muestra
 *   - si no, lo marca como sospechoso.
 *
 * @tabla  tabla hash de servicios (de port_utils)
 * @inicio puerto inicial (>=1)
 * @fin    puerto final   (<=1024)
 */
void escanear_puertos(GHashTable *tabla, int inicio, int fin);

#endif // PORT_SCANNER_H
