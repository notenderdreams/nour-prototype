#ifndef FS_H
#define FS_H

#include "arena.h"
#include <stddef.h>

typedef struct {
    char **files;
    size_t count;
} FileList;

int ensure_directory(const char *path);
FileList expand_glob(Arena *arena, const char *pattern);
FileList get_dependent_files(Arena *arena, const char *filepath);

#endif // FS_H