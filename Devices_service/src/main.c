#include "mount_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include "socket_client.h"

#define INTERVALO_ESCANEO 5
#define UMBRAL_DEFECTO 10

int main(int argc, char *argv[])
{
    int umbral = UMBRAL_DEFECTO;

    if (argc > 1) {
        umbral = atoi(argv[1]);
        if (umbral < 1 || umbral > 100) {
            fprintf(stderr, "⚠️  Umbral inválido. Debe estar entre 1 y 100.\n");
            return 1;
        }
    }

    printf("🧪 Iniciando run_monitor() con umbral de %d%%...\n", umbral);
    run_monitor(INTERVALO_ESCANEO, umbral);
    return 0;
}
