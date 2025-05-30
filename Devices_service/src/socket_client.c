#include "socket_client.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/devices.sock"

void enviar_mensaje(const char *mensaje) {
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(sockfd);
        return;
    }

    printf(">> ENVIANDO: %s", mensaje);

    ssize_t bytes = send(sockfd, mensaje, strlen(mensaje), 0);
    if (bytes < 0) {
        perror("send");
    } else {
        printf(">> %zd bytes enviados correctamente\n", bytes);
        //test_leer_socket_directamente();
    }

    close(sockfd);
}
