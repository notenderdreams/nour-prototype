#include "compile.h"
#include "nstr.h"
#include "arena.h"
#include "utils.h"
#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>



int compile_project(const Project *project) {
    int result = 1;
    Arena *arena = NULL;

    if (project == NULL || project->cc == NULL || project->build_dir == NULL || project->sources == NULL) {
        fprintf(stderr, "Invalid project configuration.\n");
        return 1;
    }

    if (ensure_directory(project->build_dir) != 0) {
        return 1;
    }

    // Create arena for string building (8KB initial)
    arena = arena_create(8192);
    if (!arena) {
        fprintf(stderr, "Failed to create arena for string building.\n");
        return 1;
    }

    const char *output_name = project->output_name != NULL ? project->output_name : project->name;
    if (output_name == NULL) {
        output_name = "sandbox_app";
    }

    // Build output path using arena
    nstr build_dir_str = nstr_from(arena, project->build_dir);
    nstr slash = nstr_from(arena, "/");
    nstr output_name_str = nstr_from(arena, output_name);
    if (build_dir_str.data == NULL || slash.data == NULL || output_name_str.data == NULL) {
        fprintf(stderr, "Failed to allocate output path components.\n");
        goto cleanup;
    }
    
    nstr output_path = nstr_concat(arena, build_dir_str, slash);
    output_path = nstr_concat(arena, output_path, output_name_str);
    if (output_path.data == NULL) {
        fprintf(stderr, "Failed to build output path.\n");
        goto cleanup;
    }

    // Start building compile command
    nstr command = nstr_from(arena, project->cc);
    command = nstr_append(arena, command, " -o");
    command = nstr_append(arena, command, " ");
    command = nstr_concat(arena, command, output_path);
    if (command.data == NULL) {
        fprintf(stderr, "Failed to build compile command.\n");
        goto cleanup;
    }

    // Add cflags
    if (project->cflags != NULL) {
        for (char **flag = project->cflags; *flag != NULL; flag++) {
            command = nstr_append(arena, command, " ");
            command = nstr_append(arena, command, *flag);
            if (command.data == NULL) {
                fprintf(stderr, "Failed to append cflags to compile command.\n");
                goto cleanup;
            }
        }
    }

    // Expand glob patterns and add source files
    printf("Resolved source files:\n");
    for (char **source = project->sources; *source != NULL; source++) {
        FileList files = expand_glob(arena, *source);
        if (files.count == 0) {
            fprintf(stderr, "No files found for pattern: %s\n", *source);
            goto cleanup;
        }
        for (size_t i = 0; i < files.count; i++) {
            printf("  %s\n", files.files[i]);
            command = nstr_append(arena, command, " ");
            command = nstr_append(arena, command, files.files[i]);
            if (command.data == NULL) {
                fprintf(stderr, "Failed to append sources to compile command.\n");
                goto cleanup;
            }
        }
    }

    printf("Compiling sandbox with command:\n%s\n", nstr_cstr(command));
    int status = system(nstr_cstr(command));

    if (status == -1) {
        perror("system failed");
        goto cleanup;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        printf("Sandbox build succeeded: %s\n", nstr_cstr(output_path));
        result = 0;
        goto cleanup;
    }

    fprintf(stderr, "Sandbox build failed with status: %d\n", status);

cleanup:
    arena_destroy(arena);
    return result;
}
