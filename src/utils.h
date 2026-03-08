#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "nour.h"
#include "log.h"

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
}

#endif // UTILS_H