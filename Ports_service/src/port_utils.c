#include "port_utils.h"
#include <glib.h>

GHashTable* inicializar_tabla_puertos(void) {
    GHashTable* tabla = g_hash_table_new(g_int_hash, g_int_equal);

static int puertos[] = {
    20, 21, 22, 23, 25, 80, 110, 143, 443,
    465, 587, 993, 995,
    2121, 2222, 2525, 8080, 8000, 8443,
    2143, 2993, 2110, 2995
};

const char* servicios[] = {
    "FTP-DATA", "FTP", "SSH", "Telnet", "SMTP", "HTTP", "POP3", "IMAP",
    "HTTPS", "SMTPS", "SMTP (STARTTLS)", "IMAPS", "POP3S",
    "FTP", "SSH", "SMTP", "HTTP", "HTTP", "HTTPS",
    "IMAP", "IMAPS", "POP3", "POP3S"
};


    int n = sizeof(puertos)/sizeof(puertos[0]);
    for (int i = 0; i < n; ++i) {
        int *clave = g_new(int, 1);
        *clave = puertos[i];
        g_hash_table_insert(tabla, clave, (gpointer)servicios[i]);
    }

    return tabla;
}

const char* buscar_servicio(GHashTable *tabla, int puerto) {
    gpointer v = g_hash_table_lookup(tabla, &puerto);
    return v ? (const char*)v : NULL;
}
