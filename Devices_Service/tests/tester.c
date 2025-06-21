#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <errno.h>

#define BASE_PATH "/mnt"
#define NUM_USB 5

int UMBRAL = 10;     // por defecto
int ESPERA = 10;     // por defecto

void imprimir_montajes_actuales() {
    printf("📂 Verificando contenido de /proc/mounts:\n");
    system("cat /proc/mounts | grep /mnt");
}

void crear_dispositivo(const char *path) {
    mkdir(BASE_PATH, 0755); // Asegura que /mnt exista
    mkdir(path, 0755);
    if (mount("tmpfs", path, "tmpfs", 0, "size=10M") == 0) {
        printf("[+] Montado: %s\n", path);
        imprimir_montajes_actuales();
    } else {
        fprintf(stderr, "✗ Error al montar %s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void poblar_contenido(const char *path, int index) {
    char archivo[256], subdir[256], subarchivo[256];
    snprintf(archivo, sizeof(archivo), "%s/file%d.txt", path, index);
    snprintf(subdir, sizeof(subdir), "%s/sub", path);
    snprintf(subarchivo, sizeof(subarchivo), "%s/otro%d.txt", subdir, index);

    mkdir(subdir, 0755);

    FILE *f = fopen(archivo, "w");
    if (f) {
        fprintf(f, "archivo original %d\n", index);
        fclose(f);
    }

    f = fopen(subarchivo, "w");
    if (f) {
        fprintf(f, "subarchivo %d\n", index);
        fclose(f);
    }

    sync();
}

void modificar_contenido(const char *path, int index) {
    char archivo[256], nuevo[256], eliminado[256];
    snprintf(archivo, sizeof(archivo), "%s/file%d.txt", path, index);
    snprintf(nuevo, sizeof(nuevo), "%s/nuevo%d.txt", path, index);
    snprintf(eliminado, sizeof(eliminado), "%s/sub/otro%d.txt", path, index);

    FILE *f = fopen(archivo, "w");
    if (f) {
        fprintf(f, "contenido modificado significativamente para detección\n");
        fclose(f);
    }

    f = fopen(nuevo, "w");
    if (f) {
        fprintf(f, "nuevo archivo agregado\n");
        fclose(f);
    }

    remove(eliminado);
    sync();
}

void desmontar_dispositivo(const char *path) {
    if (umount(path) == 0) {
        printf("[-] Desmontado: %s\n", path);
        rmdir(path);
    } else {
        fprintf(stderr, "✗ Error al desmontar %s: %s\n", path, strerror(errno));
    }
}

void procesar_argumentos(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--umbral=", 9) == 0) {
            UMBRAL = atoi(argv[i] + 9);
        } else if (strncmp(argv[i], "--espera=", 9) == 0) {
            ESPERA = atoi(argv[i] + 9);
        } else {
            fprintf(stderr, "⚠️  Argumento no reconocido: %s\n", argv[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    char path[256];

    procesar_argumentos(argc, argv);

    printf("[*] Umbral de detección configurado: %d%%\n", UMBRAL);
    printf("[*] Tiempo de espera entre fases: %d segundos\n", ESPERA);
    printf("[*] Simulando %d dispositivos USB...\n", NUM_USB);

    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        crear_dispositivo(path);
    }

    sleep(2);

    printf("[*] Poblando contenido inicial...\n");
    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        poblar_contenido(path, i);
    }

    printf("[*] Esperando detección inicial...\n");
    sleep(ESPERA);

    printf("[*] Modificando contenido para prueba de cambios...\n");
    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        modificar_contenido(path, i);
    }

    printf("[*] Esperando detección de cambios...\n");
    sleep(ESPERA);

    printf("[*] Desmontando dispositivos simulados...\n");
    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        desmontar_dispositivo(path);
    }

    printf("[✓] Test finalizado exitosamente.\n");
    return 0;
}
