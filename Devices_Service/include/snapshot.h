#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "file_info.h"

FileInfo *build_snapshot(const char *root);
void diff_snapshots(FileInfo *old, FileInfo *nuevo, int umbral_porcentaje);
void free_snapshot(FileInfo *snap);

#endif
