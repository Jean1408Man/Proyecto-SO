#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

#define NUM_TEST_PORTS 6

/**
 * Abre un socket TCP escuchando en el puerto indicado.
 * Devuelve el descriptor listening, o -1 en caso de error.
 */
int open_fake_port(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sock);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "bind(%d): %s\n", port, strerror(errno));
        close(sock);
        return -1;
    }

    if (listen(sock, 1) < 0) {
        perror("listen");
        close(sock);
        return -1;
    }

    return sock;
}

/**
 * Acepta UNA conexión en 'sockfd' y envía el banner simulado para 'port'.
 * Luego cierra la conexión y retorna 0.
 */
int send_banner(int sockfd, int port) {
    int client = accept(sockfd, NULL, NULL);
    if (client < 0) {
        perror("accept");
        return -1;
    }

    const char *banner = NULL;
    switch (port) {
        case 21:
            banner = "220 FTP fake servidor listo\r\n";
            break;
        case 22:
            banner = "SSH-2.0-OpenSSH_Test_22\r\n";
            break;
        case 25:
            banner = "220 SMTP fake servidor listo\r\n";
            break;
        case 80:
            banner = "HTTP/1.0 200 OK\r\nServer: FakeHTTP-80\r\n\r\n";
            break;
        case 31337:
            banner = "HTTP/1.0 200 OK\r\nServer: FakeHTTP-31337\r\n\r\n";
            break;
        case 4444:
            banner = "HTTP/1.0 200 OK\r\nServer: FakeHTTP-4444\r\n\r\n";
            break;
        default:
            banner = "FAKE-SERVICE-BANNER\r\n";
            break;
    }

    if (strlen(banner) > 0) {
        send(client, banner, strlen(banner), 0);
    }
    close(client);
    return 0;
}

int main(void) {
    int ports[NUM_TEST_PORTS] = {21, 22, 25, 80, 31337, 4444};
    int sockets[NUM_TEST_PORTS];
    pid_t hijos[NUM_TEST_PORTS];

    // 1) Abrir cada puerto de prueba y fork para enviar banner
    for (int i = 0; i < NUM_TEST_PORTS; i++) {
        int port = ports[i];
        sockets[i] = open_fake_port(port);
        if (sockets[i] < 0) {
            fprintf(stderr, "ERROR: no se pudo abrir puerto %d para pruebas\n", port);
            hijos[i] = -1;
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(sockets[i]);
            hijos[i] = -1;
            continue;
        }
        else if (pid == 0) {
            // Proceso hijo: atiende UNA sola conexión y envía su banner
            send_banner(sockets[i], port);
            exit(0);
        } else {
            // Proceso padre almacena PID y deja el socket en listening
            hijos[i] = pid;
        }
    }

    // 2) Padre duerme un tiempo suficiente para que el escáner se conecte a todos.
    sleep(15);

    // 3) Padre cierra sockets listening y espera que hijos terminen
    for (int i = 0; i < NUM_TEST_PORTS; i++) {
        if (sockets[i] >= 0) {
            close(sockets[i]);
        }
    }
    for (int i = 0; i < NUM_TEST_PORTS; i++) {
        if (hijos[i] > 0) {
            waitpid(hijos[i], NULL, 0);
        }
    }

    return 0;
}
