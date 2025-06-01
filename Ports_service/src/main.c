#include <stdio.h>
#include <stdlib.h>
#include "port_utils.h"
#include "port_scanner.h"

int main(void) {
    GHashTable *tabla = inicializar_tabla_puertos();
    if (tabla == NULL) {
        fprintf(stderr, "ERROR: No se pudo inicializar la tabla de puertos.\n");
        return EXIT_FAILURE;
    }

    printf("🔍 Escaneando puertos 1–1024 con %d hilos...\n", NUM_HILOS);
    escanear_puertos(tabla, 1, 1024);

    g_hash_table_destroy(tabla);
    return EXIT_SUCCESS;
}
