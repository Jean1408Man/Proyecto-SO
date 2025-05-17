#include "mount_manager.h"
#include <stdio.h>
#define INTERVALO_ESCANEO 5

int main(void)
{
    setbuf(stdout, NULL);
    run_monitor(INTERVALO_ESCANEO);
    return 0;
}

