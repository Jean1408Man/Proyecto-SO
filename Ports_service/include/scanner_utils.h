#ifndef SCANNER_UTILS_H
#define SCANNER_UTILS_H

#include <stddef.h>

/**
 * Intentar conectar al localhost:port con timeout ~1s.
 * Devuelve socket >= 0 si se conectó, o -1 si falla.
 */
int connect_to_port(int port);

/**
 * Tras conectar un socket válido, lee hasta `len` bytes en `buffer`.
 * Devuelve la cantidad de bytes leídos (0 si ningún dato en timeout), o -1 en error.
 */
int grab_banner(int sockfd, char *buffer, size_t len);

/**
 * Busca en el banner alguna palabra “peligrosa” (virus, malware, etc.).
 * Devuelve un puntero constante a la palabra encontrada, o NULL si no coincide nada.
 */
const char *search_dangerous_words(const char *banner, int n);

/**
 * Cierra de forma segura el socket dado.
 */
void close_socket(int sockfd);

#endif // SCANNER_UTILS_H
