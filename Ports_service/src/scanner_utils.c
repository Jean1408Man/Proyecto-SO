#include "scanner_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <netdb.h>

/**
 * Intentamos conectar en modo no bloqueante con select() para timeout de ~1s.
 * Una vez establecida la conexión, volvemos el socket a modo bloqueo y lo devolvemos.
 */
int connect_to_port(int port) {
    int sockfd;
    struct sockaddr_in addr;
    int flags, res, err;
    fd_set wset;
    struct timeval tv;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    // 1) Poner el socket en modo no bloqueante
    if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0) {
        close(sockfd);
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(sockfd);
        return -1;
    }

    // 2) Rellenar sockaddr_in para localhost:port
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }

    // 3) Intentar connect (no bloqueante)
    res = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0) {
        if (errno != EINPROGRESS) {
            // Error inmediato: puerto cerrado/filtrado
            close(sockfd);
            return -1;
        }
    }

    // 4) Esperar con select() a que termine el connect o timeout ~1s
    FD_ZERO(&wset);
    FD_SET(sockfd, &wset);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    res = select(sockfd + 1, NULL, &wset, NULL, &tv);
    if (res <= 0) {
        // Timeout o error
        close(sockfd);
        return -1;
    }

    // 5) Verificar que la conexión finalmente no devolvió error
    socklen_t len = sizeof(err);
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
        close(sockfd);
        return -1;
    }

    // 6) Aquí la conexión se estableció correctamente. Volvemos el socket a modo BLOQUEANTE.
    int newflags = flags & (~O_NONBLOCK);
    if (fcntl(sockfd, F_SETFL, newflags) < 0) {
        // Si no podemos volverlo a bloqueante, igual devolvemos sockfd, 
        // pero advertimos (opcional):
        // perror("fcntl: no se pudo restaurar modo bloqueante");
    }

    // 7) Devolver socket listo para recv() en modo bloqueante
    return sockfd;
}

/**
 * Lee del socket 'sockfd' hasta 'len' bytes (o hasta timeout ~1s).
 * Como socket ya está en modo BLOQUEANTE, podemos usar SO_RCVTIMEO con confianza.
 */
int grab_banner(int sockfd, char *buffer, size_t len) {
    if (sockfd < 0 || buffer == NULL || len == 0) {
        return -1;
    }

    // Poner un timeout de lectura de 1 segundo
    struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int n = recv(sockfd, buffer, len, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return 0; // no hubo banner dentro del timeout
        }
        return -1; // error real en recv
    }
    if (n == 0) {
        return 0; // el peer cerró sin enviar datos
    }
    buffer[n] = '\0';
    return n;
}

void close_socket(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
    }
}
