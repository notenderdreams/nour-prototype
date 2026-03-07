#include "compile.h"
#include "nstr.h"
#include "arena.h"
#include "utils.h"
#include "fs.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Build a .o path from a source path: sandbox/basic.c -> build/sandbox_basic.o
static nstr obj_path_for(Arena *arena, const char *build_dir, const char *source) {
    nstr result = nstr_from(arena, build_dir);
    result = nstr_append(arena, result, "/");

    // Flatten the source path: replace '/' with '_'
    size_t len = strlen(source);
    char *flat = arena_alloc(arena, len + 1);
    if (!flat) return (nstr){0, NULL};
    for (size_t i = 0; i < len; i++)
        flat[i] = (source[i] == '/') ? '_' : source[i];
    flat[len] = '\0';

    // Replace .c extension with .o
    if (len >= 2 && flat[len - 2] == '.' && flat[len - 1] == 'c') {
        flat[len - 1] = 'o';
    }

    result = nstr_append(arena, result, flat);
    return result;
}

int compile_project(const Project *project) {
    int result = 1;
    Arena *arena = NULL;

    if (project == NULL || project->cc == NULL || project->build_dir == NULL || project->sources == NULL) {
        log_print(LOG_ERROR, "Invalid project configuration.\n");
        return 1;
    }

    if (ensure_directory(project->build_dir) != 0) {
        return 1;
    }

    arena = arena_create(8192);
    if (!arena) {
        log_print(LOG_ERROR, "Failed to create arena for string building.\n");
        return 1;
    }

    const char *output_name = project->output_name != NULL ? project->output_name : project->name;
    if (output_name == NULL) {
        output_name = "sandbox_app";
    }

    // Build output path
    nstr output_path = nstr_from(arena, project->build_dir);
    output_path = nstr_append(arena, output_path, "/");
    output_path = nstr_append(arena, output_path, output_name);
    if (output_path.data == NULL) {
        log_print(LOG_ERROR, "Failed to build output path.\n");
        goto cleanup;
    }

    // Expand glob patterns, collect all resolved source files
    size_t max_sources = 256;
    char **all_sources = arena_alloc(arena, sizeof(char *) * max_sources);
    size_t all_sources_count = 0;
    if (!all_sources) goto cleanup;

    log_print(LOG_INFO, "Resolved source files:\n");
    for (char **source = project->sources; *source != NULL; source++) {
        FileList files = expand_glob(arena, *source);
        if (files.count == 0) {
            log_print(LOG_ERROR, "No files found for pattern: %s\n", *source);
            goto cleanup;
        }
        for (size_t i = 0; i < files.count; i++) {
            log_print(LOG_INFO, "  %s\n", files.files[i]);
            if (all_sources_count < max_sources)
                all_sources[all_sources_count++] = files.files[i];
        }
    }

    // Build and print the reverse dependency graph
    FileList sources_list = {all_sources, all_sources_count};
    DepGraph dep_graph = build_dep_graph(arena, sources_list);
    print_dep_graph(&dep_graph);

    // --- Phase 1: Compile each source to .o in parallel ---
    log_print(LOG_INFO, "Compiling %zu source files...\n", all_sources_count);

    nstr *obj_paths = arena_alloc(arena, sizeof(nstr) * all_sources_count);
    pid_t *pids = arena_alloc(arena, sizeof(pid_t) * all_sources_count);
    if (!obj_paths || !pids) goto cleanup;

    for (size_t i = 0; i < all_sources_count; i++) {
        obj_paths[i] = obj_path_for(arena, project->build_dir, all_sources[i]);
        if (obj_paths[i].data == NULL) {
            log_print(LOG_ERROR, "Failed to build object path for: %s\n", all_sources[i]);
            goto cleanup;
        }

        // Build compile command: cc -c -o obj cflags source
        nstr cmd = nstr_from(arena, project->cc);
        cmd = nstr_append(arena, cmd, " -c -o ");
        cmd = nstr_concat(arena, cmd, obj_paths[i]);
        if (project->cflags != NULL) {
            for (char **flag = project->cflags; *flag != NULL; flag++) {
                cmd = nstr_append(arena, cmd, " ");
                cmd = nstr_append(arena, cmd, *flag);
            }
        }
        cmd = nstr_append(arena, cmd, " ");
        cmd = nstr_append(arena, cmd, all_sources[i]);
        if (cmd.data == NULL) {
            log_print(LOG_ERROR, "Failed to build compile command for: %s\n", all_sources[i]);
            goto cleanup;
        }

        log_print(LOG_INFO, "  %s\n", nstr_cstr(cmd));

        pid_t pid = fork();
        if (pid == -1) {
            log_print(LOG_ERROR, "fork() failed for: %s\n", all_sources[i]);
            goto cleanup;
        }

        if (pid == 0) {
            // Child: exec the compile command
            _exit(system(nstr_cstr(cmd)));
        }

        pids[i] = pid;
    }

    // Wait for all compilations
    int compile_failed = 0;
    for (size_t i = 0; i < all_sources_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            log_print(LOG_ERROR, "Compilation failed: %s\n", all_sources[i]);
            compile_failed = 1;
        } else {
            log_print(LOG_OK, "Compiled: %s\n", all_sources[i]);
        }
    }

    if (compile_failed) {
        log_print(LOG_ERROR, "One or more files failed to compile.\n");
        goto cleanup;
    }

    // --- Phase 2: Link all .o files ---
    {
        nstr link_cmd = nstr_from(arena, project->cc);
        link_cmd = nstr_append(arena, link_cmd, " -o ");
        link_cmd = nstr_concat(arena, link_cmd, output_path);

        for (size_t i = 0; i < all_sources_count; i++) {
            link_cmd = nstr_append(arena, link_cmd, " ");
            link_cmd = nstr_concat(arena, link_cmd, obj_paths[i]);
        }

        if (link_cmd.data == NULL) {
            log_print(LOG_ERROR, "Failed to build link command.\n");
            goto cleanup;
        }

        log_print(LOG_INFO, "Linking: %s\n", nstr_cstr(link_cmd));
        int status = system(nstr_cstr(link_cmd));

        if (status == -1) {
            perror("system failed");
            goto cleanup;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            log_print(LOG_OK, "Build succeeded: %s\n", nstr_cstr(output_path));
            result = 0;
            goto cleanup;
        }

        log_print(LOG_ERROR, "Linking failed with status: %d\n", status);
    }

cleanup:
    arena_destroy(arena);
    return result;
}
