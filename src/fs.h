#ifndef FS_H
#define FS_H

#include "arena.h"
#include <stddef.h>
#include <time.h>

typedef struct {
    char **files;
    size_t count;
} FileList;

typedef struct {
    char *file;
    char **dependents;
    size_t count;
} DepNode;

typedef struct {
    DepNode *nodes;
    size_t count;
} DepGraph;

int ensure_directory(const char *path);
time_t get_mtime(const char *path);
FileList expand_glob(Arena *arena, const char *pattern);
FileList get_dependent_files(Arena *arena, const char *filepath);
DepGraph build_dep_graph(Arena *arena, FileList sources);
void print_tree(char **items, size_t count, const char *padding);
void print_dep_graph(const DepGraph *graph);

#endif // FS_H