#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>

#define BASE_PATH "/mnt"
#define NUM_USB 5

void crear_dispositivo(const char *path) {
    mkdir(path, 0755);
    if (mount("tmpfs", path, "tmpfs", 0, "size=10M") == 0) {
        printf("[+] Montado: %s\n", path);
    } else {
        perror("Error al montar tmpfs");
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
    if (f) { fprintf(f, "archivo original %d\n", index); fclose(f); }

    f = fopen(subarchivo, "w");
    if (f) { fprintf(f, "subarchivo %d\n", index); fclose(f); }
}

void modificar_contenido(const char *path, int index) {
    char archivo[256], nuevo[256], eliminado[256];
    snprintf(archivo, sizeof(archivo), "%s/file%d.txt", path, index);
    snprintf(nuevo, sizeof(nuevo), "%s/nuevo%d.txt", path, index);
    snprintf(eliminado, sizeof(eliminado), "%s/sub/otro%d.txt", path, index);

    FILE *f = fopen(archivo, "w");
    if (f) { fprintf(f, "contenido modificado\n"); fclose(f); }

    f = fopen(nuevo, "w");
    if (f) { fprintf(f, "nuevo archivo\n"); fclose(f); }

    remove(eliminado);
}

void desmontar_dispositivo(const char *path) {
    if (umount(path) == 0) {
        printf("[-] Desmontado: %s\n", path);
        rmdir(path);
    } else {
        perror("Error al desmontar dispositivo");
    }
}

int main() {
    char path[256];

    printf("[*] Simulando %d dispositivos USB...\n", NUM_USB);
    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        crear_dispositivo(path);
    }

    sleep(2);

    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        poblar_contenido(path, i);
    }

    printf("[*] Esperando detección inicial...\n");
    sleep(10);

    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        modificar_contenido(path, i);
    }

    printf("[*] Esperando detección de cambios...\n");
    sleep(10);

    for (int i = 1; i <= NUM_USB; ++i) {
        snprintf(path, sizeof(path), "%s/usb%d", BASE_PATH, i);
        desmontar_dispositivo(path);
    }

    printf("[✓] Test finalizado exitosamente.\n");
    return 0;
}