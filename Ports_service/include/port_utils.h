#ifndef PORT_UTILS_H
#define PORT_UTILS_H

#include <glib.h>

/**
 * Inicializa y devuelve una GHashTable* con los puertos comunes (1–1024)
 * mapeados a su nombre de servicio.
 * El caller debe destruirla con g_hash_table_destroy().
 */
GHashTable* inicializar_tabla_puertos(void);

/**
 * Busca en la tabla el servicio asociado a @puerto.
 * @return nombre del servicio (string constante) o NULL si no existe.
 */
const char* buscar_servicio(GHashTable *tabla, int puerto);

#endif // PORT_UTILS_H
