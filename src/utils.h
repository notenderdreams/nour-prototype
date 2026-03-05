#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include "nour.h"
#include <errno.h>
#include <sys/stat.h>

static inline void print_project(Project *project) {
    printf("name: %s\n", project->name);
    printf("version: %s\n", project->version);
    printf("cc: %s\n", project->cc);
    printf("build_dir: %s\n", project->build_dir);
    printf("output_name: %s\n", project->output_name);

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

static int ensure_directory(const char *path) {
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    perror("mkdir failed");
    return -1;
}

#endif // UTILS_H