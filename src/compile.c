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

    size_t len = strlen(source);
    char *flat = arena_alloc(arena, len + 1);
    if (!flat) return (nstr){0, NULL};
    for (size_t i = 0; i < len; i++)
        flat[i] = (source[i] == '/') ? '_' : source[i];
    flat[len] = '\0';

    if (len >= 2 && flat[len - 2] == '.' && flat[len - 1] == 'c') {
        flat[len - 1] = 'o';
    }

    result = nstr_append(arena, result, flat);
    return result;
}

// Count NULL-terminated cflags array
static size_t count_cflags(char **cflags) {
    size_t n = 0;
    if (cflags) {
        for (char **f = cflags; *f != NULL; f++) n++;
    }
    return n;
}

// Log an argv array as a single command line
static void log_argv(LogLevel level, char **argv) {
    // Print first arg
    if (!argv || !argv[0]) return;
    log_print(level, "%s", argv[0]);
    for (size_t i = 1; argv[i] != NULL; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
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
            log_print(LOG_INFO, "    %s\n", files.files[i]);
            if (all_sources_count < max_sources)
                all_sources[all_sources_count++] = files.files[i];
        }
    }

    // Build and print the reverse dependency graph
    FileList sources_list = {all_sources, all_sources_count};
    DepGraph dep_graph = build_dep_graph(arena, sources_list);
    print_dep_graph(&dep_graph);

    // --- Phase 1: Compile each source to .o in parallel via execvp ---
    long max_jobs = sysconf(_SC_NPROCESSORS_ONLN);
    if (max_jobs < 1) max_jobs = 1;
    log_print(LOG_INFO, "Available parallel jobs : %ld\n", max_jobs);
    log_print(LOG_INFO, "Compiling %zu source files\n", all_sources_count);

    size_t nflags = count_cflags(project->cflags);
    nstr *obj_paths = arena_alloc(arena, sizeof(nstr) * all_sources_count);
    pid_t *pids = arena_alloc(arena, sizeof(pid_t) * all_sources_count);
    if (!obj_paths || !pids) goto cleanup;

    size_t launched = 0;
    size_t reaped = 0;
    int compile_failed = 0;

    while (reaped < all_sources_count) {
        // Launch up to max_jobs concurrent compilations
        while (launched < all_sources_count && (long)(launched - reaped) < max_jobs) {
            size_t i = launched;

            obj_paths[i] = obj_path_for(arena, project->build_dir, all_sources[i]);
            if (obj_paths[i].data == NULL) {
                log_print(LOG_ERROR, "Failed to build object path for: %s\n", all_sources[i]);
                goto cleanup;
            }

            // Build argv: [cc, -c, -o, obj, ...cflags, source, NULL]
            size_t argc = 4 + nflags + 1;
            char **argv = arena_alloc(arena, sizeof(char *) * (argc + 1));
            if (!argv) goto cleanup;

            size_t a = 0;
            argv[a++] = project->cc;
            argv[a++] = "-c";
            argv[a++] = "-o";
            argv[a++] = (char *)nstr_cstr(obj_paths[i]);
            for (size_t f = 0; f < nflags; f++)
                argv[a++] = project->cflags[f];
            argv[a++] = all_sources[i];
            argv[a] = NULL;

            log_argv(LOG_INFO, argv);

            pid_t pid = fork();
            if (pid == -1) {
                log_print(LOG_ERROR, "fork() failed for: %s\n", all_sources[i]);
                goto cleanup;
            }

            if (pid == 0) {
                execvp(argv[0], argv);
                perror("execvp");
                _exit(1);
            }

            pids[i] = pid;
            launched++;
        }

        // Wait for any child to finish
        int status;
        pid_t done = waitpid(-1, &status, 0);
        if (done == -1) break;

        // Find which source this pid belongs to
        for (size_t i = 0; i < launched; i++) {
            if (pids[i] == done) {
                if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                    log_print(LOG_ERROR, "Compilation failed: %s\n", all_sources[i]);
                    compile_failed = 1;
                } else {
                    log_print(LOG_OK, "Compiled: %s\n", all_sources[i]);
                }
                pids[i] = 0; // mark as reaped
                reaped++;
                break;
            }
        }
    }

    if (compile_failed) {
        log_print(LOG_ERROR, "One or more files failed to compile.\n");
        goto cleanup;
    }

    // --- Phase 2: Link all .o files via execvp ---
    {
        // argv: [cc, -o, output, ...objs, NULL]
        size_t argc = 3 + all_sources_count;
        char **argv = arena_alloc(arena, sizeof(char *) * (argc + 1));
        if (!argv) goto cleanup;

        size_t a = 0;
        argv[a++] = project->cc;
        argv[a++] = "-o";
        argv[a++] = (char *)nstr_cstr(output_path);
        for (size_t i = 0; i < all_sources_count; i++)
            argv[a++] = (char *)nstr_cstr(obj_paths[i]);
        argv[a] = NULL;

        log_argv(LOG_INFO, argv);

        pid_t pid = fork();
        if (pid == -1) {
            log_print(LOG_ERROR, "fork() failed for linking.\n");
            goto cleanup;
        }

        if (pid == 0) {
            execvp(argv[0], argv);
            perror("execvp");
            _exit(1);
        }

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            log_print(LOG_OK, "Build succeeded: %s\n", nstr_cstr(output_path));
            result = 0;
            goto cleanup;
        }

        log_print(LOG_ERROR, "Linking failed with status: %d\n", WEXITSTATUS(status));
    }

cleanup:
    arena_destroy(arena);
    return result;
}
