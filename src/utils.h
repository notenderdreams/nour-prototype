#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "nour.h"

static inline void print_project(const Project *project) {
    if (!project) return;
    printf("name: %s\n", project->name ? project->name : "(null)");
    printf("version: %s\n", project->version ? project->version : "(null)");
    printf("cc: %s\n", project->cc ? project->cc : "(null)");
    printf("build_dir: %s\n", project->build_dir ? project->build_dir : "(null)");
    printf("output_name: %s\n", project->output_name ? project->output_name : "(null)");

    if (project->cflags != NULL) {
        printf("cflags:\n");
        for (char **flag = project->cflags; *flag != NULL; flag++) {
            printf("%s\n", *flag);
        }
    }

    printf("sources:\n");
    for (char **source = project->sources; source != NULL && *source != NULL; source++) {
        printf("- %s\n", *source);
    }
}

#endif // UTILS_H