#ifndef FILE_INFO_H
#define FILE_INFO_H

#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "uthash.h"

/* --- Foto de un fichero --- */
typedef struct file_info {
    char   *rel_path;          /* clave */
    uint8_t sha256[32];        /* hash SHA-256 */
    time_t  mtime;             /* fecha de modificación */
    off_t   size;              /* tamaño del archivo */
    mode_t  mode;              /* permisos y tipo de archivo */
    uid_t   uid;               /* ID de usuario (dueño) */
    gid_t   gid;               /* ID de grupo */
    UT_hash_handle hh;         /* enlace uthash */
} FileInfo;

#endif /* FILE_INFO_H */
