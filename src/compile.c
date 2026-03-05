#include "compile.h"
#include "utils.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define COMMAND_MAX 8192

static int append_token(char *command, size_t command_size, const char *token) {
    size_t used = strlen(command);
    int written = snprintf(command + used, command_size - used, " %s", token);
    return (written < 0 || (size_t)written >= command_size - used) ? -1 : 0;
}

int compile_project(const Project *project) {
    if (project == NULL || project->cc == NULL || project->build_dir == NULL || project->sources == NULL) {
        fprintf(stderr, "Invalid project configuration.\n");
        return 1;
    }

    if (ensure_directory(project->build_dir) != 0) {
        return 1;
    }

    const char *output_name = project->output_name != NULL ? project->output_name : project->name;
    if (output_name == NULL) {
        output_name = "sandbox_app";
    }

    char output_path[PATH_MAX];
    if (snprintf(output_path, sizeof(output_path), "%s/%s", project->build_dir, output_name) >= (int)sizeof(output_path)) {
        fprintf(stderr, "Output path is too long.\n");
        return 1;
    }

    char command[COMMAND_MAX] = {0};
    if (snprintf(command, sizeof(command), "%s -o %s", project->cc, output_path) >= (int)sizeof(command)) {
        fprintf(stderr, "Compile command is too long.\n");
        return 1;
    }

    if (project->cflags != NULL) {
        for (char **flag = project->cflags; *flag != NULL; flag++) {
            if (append_token(command, sizeof(command), *flag) != 0) {
                fprintf(stderr, "Compile command exceeded max length while adding cflags.\n");
                return 1;
            }
        }
    }

    for (char **source = project->sources; *source != NULL; source++) {
        if (append_token(command, sizeof(command), *source) != 0) {
            fprintf(stderr, "Compile command exceeded max length while adding sources.\n");
            return 1;
        }
    }

    printf("Compiling sandbox with command:\n%s\n", command);
    int status = system(command);
    if (status == -1) {
        perror("system failed");
        return 1;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("Sandbox build succeeded: %s\n", output_path);
        return 0;
    }

    fprintf(stderr, "Sandbox build failed with status: %d\n", status);
    return 1;
}
