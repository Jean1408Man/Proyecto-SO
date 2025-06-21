// cpu_stress.c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

// TEST DE CPU 

void burn_cpu() {
    while (1) {
        // Cálculo pesado para ocupar CPU
        double x = 0.0001;
        for (int i = 0; i < 1000000; ++i) {
            x += x * x;
        }
    }
}

int main() {
    int num_procesos = 7; // Cambia esto según cuánta carga quieras

    for (int i = 0; i < num_procesos; ++i) {
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo
            burn_cpu();
            exit(0); // Nunca se ejecuta, pero por si acaso
        }
    }

    // El proceso padre espera (o podrías usar sleep infinito)
    while (1) {
        sleep(10);
    }

    return 0;
}
