#include "loader.h"
#include <dlfcn.h>
#include <stdio.h>


Project* load_project(const char *lib_path) {
    void *handle = dlopen(lib_path, RTLD_NOW);
    if (!handle) {
        fprintf(stderr, "Failed to load shared object: %s\n", dlerror());
        return NULL;
    }

    Project *project_ptr = (Project *)dlsym(handle, "project");
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "Failed to find symbol 'project': %s\n", error);
        dlclose(handle);
        return NULL;
    }

    return project_ptr;
}