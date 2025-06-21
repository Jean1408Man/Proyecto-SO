#include "mount_manager.h"
#include <stdio.h>
#define INTERVALO_ESCANEO 5
#include "socket_client.h"


int main(void)
{
    printf("🧪 Iniciando run_monitor()...\n");

    run_monitor(INTERVALO_ESCANEO);
    return 0;
}

