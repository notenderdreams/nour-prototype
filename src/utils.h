#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "nour.h"
#include "log.h"

static inline void print_project(const Project *project, const char *name) {
    if (!project) return;
    log_print(LOG_INFO, "Project:");
    log_print(LOG_ALIGNED, "name: %s", name ? name : "(null)");
    log_print(LOG_ALIGNED, "version: %s", project->version ? project->version : "(null)");
    log_print(LOG_ALIGNED, "cc: %s", project->cc ? project->cc : "(null)");
    log_print(LOG_ALIGNED, "build_dir: %s", project->build_dir ? project->build_dir : "(null)");

    if (project->cflags != NULL) {
        log_print(LOG_ALIGNED, "cflags:");
        for (char **flag = project->cflags; *flag != NULL; flag++) {
            log_print(LOG_ALIGNED, "%s", *flag);
        }
    }

    log_print(LOG_ALIGNED, "sources:");
    for (char **source = project->sources; source != NULL && *source != NULL; source++) {
        log_print(LOG_ALIGNED, "- %s", *source);
    }
}

#endif // UTILS_H