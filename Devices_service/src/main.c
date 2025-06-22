#include "../include/mount_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include "socket_client.h"

#define UMBRAL_DEFECTO 10
#define INTERVALO_DEFECTO 5

int main(int argc, char *argv[])
{
    int umbral = UMBRAL_DEFECTO;
    int intervalo = INTERVALO_DEFECTO;

    if (argc >= 2) {
        umbral = atoi(argv[1]);
        if (umbral < 1 || umbral > 100) {
            fprintf(stderr, "⚠️  Umbral inválido. Debe estar entre 1 y 100.\n");
            return 1;
        }
    }

    if (argc >= 3) {
        intervalo = atoi(argv[2]);
        if (intervalo < 1 || intervalo > 3600) {
            fprintf(stderr, "⚠️  Intervalo inválido. Debe estar entre 1 y 3600 segundos.\n");
            return 1;
        }
    }

    printf("🧪 Iniciando run_monitor() con umbral: %d%%, intervalo: %d segundos...\n", umbral, intervalo);
    run_monitor(intervalo, umbral);
    return 0;
}
