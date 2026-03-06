#ifndef LOADER_H
#define LOADER_H

#include "nour.h"

typedef struct {
    Project *project;
    void *handle;
} LoadedProject;

LoadedProject load_project(const char *lib_path);
void unload_project(LoadedProject *lp);

#endif // LOADER_H