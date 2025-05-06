#define _XOPEN_SOURCE 700
#include "snapshot.h"
#include <stdio.h>        /* ← FILE, fopen, fread, fclose */
#include <ftw.h>
#include <openssl/sha.h>
#include <stdlib.h>
#include <string.h>

static FileInfo *tabla = NULL;
static const char *base_root;

static int calc_sha(const char *p, uint8_t h[32])
{
    FILE *f = fopen(p, "rb"); if (!f) return -1;
    SHA256_CTX c; SHA256_Init(&c);
    uint8_t buf[4096]; size_t r;
    while ((r = fread(buf,1,sizeof buf,f))) SHA256_Update(&c, buf, r);
    fclose(f); SHA256_Final(h, &c); return 0;
}

static int cb(const char *fpath, const struct stat *st, int tflag, struct FTW *b)
{
    if (tflag != FTW_F) return 0;
    uint8_t h[32]; if (calc_sha(fpath, h)) return 0;

    const char *rel = fpath + strlen(base_root) + 1;   /* +1 para saltar '/' */
    FileInfo *fi = malloc(sizeof *fi);
    fi->rel_path = strdup(rel);
    memcpy(fi->sha256, h, 32);
    fi->mtime = st->st_mtime;
    HASH_ADD_KEYPTR(hh, tabla, fi->rel_path, strlen(fi->rel_path), fi);
    return 0;
}

FileInfo *build_snapshot(const char *root)
{
    tabla = NULL; base_root = root;
    nftw(root, cb, 16, FTW_PHYS);
    return tabla;
}

void diff_snapshots(FileInfo *old, FileInfo *nw)
{
    FileInfo *cur, *tmp;
    HASH_ITER(hh, nw, cur, tmp) {
        FileInfo *prev; HASH_FIND_STR(old, cur->rel_path, prev);
        if (!prev)
            printf("[+] %s\n", cur->rel_path);
        else if (memcmp(prev->sha256, cur->sha256, 32))
            printf("[*] %s\n", cur->rel_path);
    }
    HASH_ITER(hh, old, cur, tmp) 
    {
        FileInfo *probe;
        HASH_FIND_STR(nw, cur->rel_path, probe);
        if (!probe)
            printf("[-] %s\n", cur->rel_path);
    }
        
}

void free_snapshot(FileInfo *snap)
{
    FileInfo *cur, *tmp;
    HASH_ITER(hh, snap, cur, tmp) {
        HASH_DEL(snap, cur);
        free(cur->rel_path); free(cur);
    }
}
