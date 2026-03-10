#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "nour.h"
#include "log.h"

static inline const char *library_type_str(LibraryType t) {
    switch (t) {
        case STATIC: return "static";
        case SHARED: return "shared";
        default:         return "unknown";
    }
}

static inline void print_executable(const Executable *exe, const char *name) {
    if (!exe) return;
    const char *n = exe->name ? exe->name : (name ? name : "?");
    log_print(LOG_INFO, "Executable: %s", n);
    if (exe->sources) {
        log_print(LOG_ALIGNED, "sources:");
        for (char **s = exe->sources; *s; s++)
            log_print(LOG_ALIGNED, "  %s", *s);
    }
    if (exe->includes) {
        log_print(LOG_ALIGNED, "includes:");
        for (char **s = exe->includes; *s; s++)
            log_print(LOG_ALIGNED, "  %s", *s);
    }
}

static inline void print_library(const Library *lib, const char *name) {
    if (!lib) return;
    const char *n = lib->name ? lib->name : (name ? name : "?");
    log_print(LOG_INFO, "Library: %s (%s)", n,
              library_type_str(lib->type));
    if (lib->sources) {
        log_print(LOG_ALIGNED, "sources:");
        for (char **s = lib->sources; *s; s++)
            log_print(LOG_ALIGNED, "  %s", *s);
    }
    if (lib->includes) {
        log_print(LOG_ALIGNED, "includes:");
        for (char **s = lib->includes; *s; s++)
            log_print(LOG_ALIGNED, "  %s", *s);
    }
}

static inline void print_target(const void *target, const char *name) {
    if (!target) return;
    TargetKind kind = *(const TargetKind *)target;
    switch (kind) {
        case TARGET_EXECUTABLE: print_executable((const Executable *)target, name); break;
        case TARGET_LIBRARY:    print_library((const Library *)target, name);       break;
    }
}

static inline void print_project(const Project *project, const char *name) {
    if (!project) return;
    log_print(LOG_INFO, "Project %s v%s (%s)",
              name ? name : "?",
              project->version ? project->version : "?",
              project->cc ? project->cc : "gcc");
    if (project->linker && project->linker[0])
        log_print(LOG_ALIGNED, "linker: %s", project->linker);

    if (project->cflags != NULL) {
        log_print(LOG_ALIGNED, "cflags:");
        for (char **flag = project->cflags; *flag != NULL; flag++) {
            log_print(LOG_ALIGNED, "%s", *flag);
        }
    }

    if (project->targets != NULL) {
        log_print(LOG_ALIGNED, "targets:");
        for (void **t = project->targets; *t; t++)
            print_target(*t, NULL);
    }
}

#endif // UTILS_H