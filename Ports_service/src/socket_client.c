#include "socket_client.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/ports.sock"

void send_msg(const char *mensaje) {
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

    ssize_t bytes = send(sockfd, mensaje, strlen(mensaje), 0);
    if (bytes < 0) {
        perror("send");
    }

    close(sockfd);
}
