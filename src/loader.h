#ifndef LOADER_H
#define LOADER_H

#include "nour.h"

typedef struct {
    Project    *project;
    void       *handle;
    const char *name;    // symbol name extracted from .nour, e.g. "test_build"
} LoadedProject;

LoadedProject load_project(const char *lib_path, const char *symbol_name);
void unload_project(LoadedProject *lp);

#endif // LOADER_H