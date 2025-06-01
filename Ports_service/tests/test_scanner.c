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
            banner = "220 Servicio FTP listo\r\n";
            break;
        case 22:
            banner = "SSH-2.0-OpenSSH_7.9\r\n";
            break;
        case 25:
            banner = "220 Servicio SMTP listo\r\n";
            break;
        case 443:
            // HTTPS no envía texto esperable en cleartext, 
            // con que la conexión TCP exista ya es “válido”
            banner = "";
            break;
        case 31337:
            // HTTP en puerto no estándar
            banner = "HTTP/1.0 200 OK\r\nServer: FakeHTTP\r\n\r\n";
            break;
        case 4444:
            // Otro HTTP en puerto alto
            banner = "HTTP/1.0 200 OK\r\nServer: FakeHTTP-Alta\r\n\r\n";
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
    int ports[NUM_TEST_PORTS] = {21, 22, 25, 443, 31337, 4444};
    int sockets[NUM_TEST_PORTS];

    for (int i = 0; i < NUM_TEST_PORTS; i++) {
        int port = ports[i];
        sockets[i] = open_fake_port(port);
        if (sockets[i] < 0) {
            fprintf(stderr, "ERROR: no se pudo abrir puerto %d para pruebas\n", port);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(sockets[i]);
            continue;
        }
        else if (pid == 0) {
            // Proceso hijo: atiende UNA sola conexión y envía su banner
            send_banner(sockets[i], port);
            exit(0);
        }
        // El proceso padre sigue y deja el socket en listening.
    }

    // Padre duerme un tiempo suficiente para que el escáner se conecte a todos.
    sleep(20);

    // Cierra sockets listening y espera que hijos terminen
    for (int i = 0; i < NUM_TEST_PORTS; i++) {
        if (sockets[i] >= 0) {
            close(sockets[i]);
        }
    }
    while (wait(NULL) > 0) { /* nada */ }

    return 0;
}
