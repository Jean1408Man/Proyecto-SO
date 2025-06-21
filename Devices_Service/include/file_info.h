#ifndef FILE_INFO_H
#define FILE_INFO_H

#include <stdint.h>
#include <time.h>
#include "uthash.h"

/* --- Foto de un fichero --- */
typedef struct file_info {
    char   *rel_path;          /* clave */
    uint8_t sha256[32];        /* hash  */
    time_t  mtime;
    UT_hash_handle hh;         /* enlace uthash */
} FileInfo;

#endif /* FILE_INFO_H */
