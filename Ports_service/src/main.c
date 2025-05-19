#include <stdio.h>
#include "port_utils.h"
#include "port_scanner.h"

int main(void) {
    GHashTable *tabla = inicializar_tabla_puertos();
    printf("🔍 Escaneando puertos 1–1024 con %d hilos...\n", NUM_HILOS);
    escanear_puertos(tabla, 1, 1024);
    g_hash_table_destroy(tabla);
    return 0;
}
