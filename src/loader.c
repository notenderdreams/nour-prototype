#include "loader.h"
#include <dlfcn.h>
#include <stdio.h>

LoadedProject load_project(const char *lib_path) {
    LoadedProject lp = {NULL, NULL};

    void *handle = dlopen(lib_path, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to load shared object: %s\n", dlerror());
        return lp;
    }

    dlerror(); 
    Project *project_ptr = (Project *)dlsym(handle, "project");
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Failed to find symbol 'project': %s\n", error);
        dlclose(handle);
        return lp;
    }

    lp.project = project_ptr;
    lp.handle = handle;
    return lp;
}

void unload_project(LoadedProject *lp) {
    if (!lp) return;
    if (lp->handle) {
        dlclose(lp->handle);
        lp->handle = NULL;
    }
    lp->project = NULL;
}
