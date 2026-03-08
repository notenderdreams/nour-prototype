#include "loader.h"
#include "log.h"
#include <dlfcn.h>
#include <stdio.h>

LoadedProject load_project(const char *lib_path, const char *symbol_name) {
    LoadedProject lp = {NULL, NULL, NULL};

    void *handle = dlopen(lib_path, RTLD_NOW);
    if (!handle) {
        log_print(LOG_ERROR, "Failed to load shared object: %s\n", dlerror());
        return lp;
    }

    dlerror();
    Project *project_ptr = (Project *)dlsym(handle, symbol_name);
    char *error = dlerror();
    if (error != NULL) {
        log_print(LOG_ERROR, "Failed to find symbol '%s': %s\n", symbol_name, error);
        dlclose(handle);
        return lp;
    }

    lp.project = project_ptr;
    lp.handle  = handle;
    lp.name    = symbol_name;
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
