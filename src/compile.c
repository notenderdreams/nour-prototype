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
    char buffer[4096];
    size_t used = 0;

    if (!argv || !argv[0]) return;

    for (size_t i = 0; argv[i] != NULL; i++) {
        int written = snprintf(buffer + used,
                               sizeof(buffer) - used,
                               "%s%s",
                               i == 0 ? "" : " ",
                               argv[i]);
        if (written < 0 || (size_t)written >= sizeof(buffer) - used) {
            log_print(level, "%s", argv[0]);
            for (size_t j = 1; argv[j] != NULL; j++) {
                log_print(LOG_ALIGNED, "%s", argv[j]);
            }
            return;
        }
        used += (size_t)written;
    }

    log_print(level, "%s", buffer);
}

// Expand OptLevel to flag string
static const char *opt_to_flag(OptLevel opt) {
    switch (opt) {
        case OPT_NONE:       return "-O0";
        case OPT_DEBUG:      return "-Og";
        case OPT_RELEASE:    return "-O2";
        case OPT_SIZE:       return "-Os";
        case OPT_AGGRESSIVE: return "-O3";
        default:             return "-O0";
    }
}

// Expand Warnings bitmask to flag array. Returns count written.
static size_t warnings_to_flags(Warnings w, const char **out, size_t max) {
    size_t n = 0;
    if (w & WARN_ALL      && n < max) out[n++] = "-Wall";
    if (w & WARN_EXTRA    && n < max) out[n++] = "-Wextra";
    if (w & WARN_ERROR    && n < max) out[n++] = "-Werror";
    if (w & WARN_PEDANTIC && n < max) out[n++] = "-Wpedantic";
    return n;
}

// Expand Sanitizers bitmask to flag array. Returns count written.
static size_t sanitizers_to_flags(Sanitizers s, const char **out, size_t max) {
    size_t n = 0;
    if (s & SAN_ADDRESS && n < max) out[n++] = "-fsanitize=address";
    if (s & SAN_UB      && n < max) out[n++] = "-fsanitize=undefined";
    if (s & SAN_THREAD  && n < max) out[n++] = "-fsanitize=thread";
    if (s & SAN_MEMORY  && n < max) out[n++] = "-fsanitize=memory";
    return n;
}

int compile_project(const Project *project, const char *name) {
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

    const char *output_name = name;

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

    for (char **source = project->sources; *source != NULL; source++) {
        FileList files = expand_glob(arena, *source);
        if (files.count == 0) {
            log_print(LOG_ERROR, "No files found for pattern: %s\n", *source);
            goto cleanup;
        }
        for (size_t i = 0; i < files.count; i++) {
            if (all_sources_count < max_sources)
                all_sources[all_sources_count++] = files.files[i];
        }
    }

    log_print(LOG_INFO, "Source files(%zu):", all_sources_count);
    for (size_t i = 0; i < all_sources_count; i++)
        log_print(LOG_ALIGNED, "%s", all_sources[i]);

    // Build and print the reverse dependency graph
    FileList sources_list = {all_sources, all_sources_count};
    DepGraph dep_graph = build_dep_graph(arena, sources_list);
    print_dep_graph(&dep_graph);

    // --- Determine which sources need recompilation ---
    size_t nflags = count_cflags(project->cflags);
    nstr *obj_paths = arena_alloc(arena, sizeof(nstr) * all_sources_count);
    int *needs_build = arena_alloc(arena, sizeof(int) * all_sources_count);
    if (!obj_paths || !needs_build) goto cleanup;

    for (size_t i = 0; i < all_sources_count; i++) {
        obj_paths[i] = obj_path_for(arena, project->build_dir, all_sources[i]);
        if (obj_paths[i].data == NULL) {
            log_print(LOG_ERROR, "Failed to build object path for: %s\n", all_sources[i]);
            goto cleanup;
        }
        needs_build[i] = 0;
    }

    // Check each source: if source or any of its #include deps is newer than .o
    for (size_t i = 0; i < all_sources_count; i++) {
        time_t obj_mtime = get_mtime(nstr_cstr(obj_paths[i]));
        if (obj_mtime == 0) {
            needs_build[i] = 1; // .o doesn't exist
            continue;
        }
        // Source itself changed?
        if (get_mtime(all_sources[i]) > obj_mtime) {
            needs_build[i] = 1;
            continue;
        }
        // Any header dependency changed?
        FileList deps = get_dependent_files(arena, all_sources[i]);
        for (size_t d = 0; d < deps.count; d++) {
            if (get_mtime(deps.files[d]) > obj_mtime) {
                needs_build[i] = 1;
                break;
            }
        }
    }

    // Propagate: if a header changed, also mark all sources that depend on it
    // (dep_graph maps header -> list of source files that include it)
    for (size_t n = 0; n < dep_graph.count; n++) {
        // Check if this header is newer than any of its dependents' .o files
        time_t header_mtime = get_mtime(dep_graph.nodes[n].file);
        if (header_mtime == 0) continue;
        for (size_t d = 0; d < dep_graph.nodes[n].count; d++) {
            // Find the index of this dependent source in all_sources
            for (size_t i = 0; i < all_sources_count; i++) {
                if (strcmp(all_sources[i], dep_graph.nodes[n].dependents[d]) == 0) {
                    time_t obj_mt = get_mtime(nstr_cstr(obj_paths[i]));
                    if (obj_mt == 0 || header_mtime > obj_mt) {
                        needs_build[i] = 1;
                    }
                    break;
                }
            }
        }
    }

    // Count how many need rebuilding
    size_t rebuild_count = 0;
    for (size_t i = 0; i < all_sources_count; i++) {
        if (needs_build[i]) rebuild_count++;
    }

    // --- Phase 1: Compile only stale sources in parallel via execvp ---
    if (rebuild_count == 0) {
        log_print(LOG_OK, "All object files up to date, nothing to compile.\n");
    } else {
        long max_jobs = sysconf(_SC_NPROCESSORS_ONLN);
        if (max_jobs < 1) max_jobs = 1;
        log_print(LOG_INFO, "Recompiling %zu/%zu files (%ld jobs)...\n", rebuild_count, all_sources_count, max_jobs);

        pid_t *pids = arena_alloc(arena, sizeof(pid_t) * all_sources_count);
        if (!pids) goto cleanup;
        for (size_t i = 0; i < all_sources_count; i++) pids[i] = 0;

        size_t next = 0;       // next source index to consider launching
        size_t reaped = 0;
        size_t in_flight = 0;
        int compile_failed = 0;

        while (reaped < rebuild_count) {
            // Launch jobs up to max_jobs
            while (next < all_sources_count && (long)in_flight < max_jobs) {
                if (!needs_build[next]) { next++; continue; }
                size_t i = next;

                // Expand enum flags
                const char *expanded_flags[32];
                size_t exp_count = 0;
                expanded_flags[exp_count++] = opt_to_flag(project->optimize);
                exp_count += warnings_to_flags(project->warnings, expanded_flags + exp_count, 32 - exp_count);
                exp_count += sanitizers_to_flags(project->sanitizers, expanded_flags + exp_count, 32 - exp_count);

                // Build argv: [cc, -c, -o, obj, ...expanded, ...cflags, source, NULL]
                size_t argc = 4 + exp_count + nflags + 1;
                char **argv = arena_alloc(arena, sizeof(char *) * (argc + 1));
                if (!argv) goto cleanup;

                size_t a = 0;
                argv[a++] = project->cc;
                argv[a++] = "-c";
                argv[a++] = "-o";
                argv[a++] = (char *)nstr_cstr(obj_paths[i]);
                for (size_t e = 0; e < exp_count; e++)
                    argv[a++] = (char *)expanded_flags[e];
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
                in_flight++;
                next++;
            }

            // Wait for any child to finish
            int status;
            pid_t done = waitpid(-1, &status, 0);
            if (done == -1) break;

            for (size_t i = 0; i < all_sources_count; i++) {
                if (pids[i] == done) {
                    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
                        log_print(LOG_ERROR, "Compilation failed: %s\n", all_sources[i]);
                        compile_failed = 1;
                    } else {
                        log_print(LOG_OK, "Compiled: %s\n", all_sources[i]);
                    }
                    pids[i] = 0;
                    in_flight--;
                    reaped++;
                    break;
                }
            }
        }

        if (compile_failed) {
            log_print(LOG_ERROR, "One or more files failed to compile.\n");
            goto cleanup;
        }
    }

    // --- Phase 2: Link if needed ---
    {
        // Check if we need to re-link: any recompilation happened, or binary missing/stale
        int needs_link = (rebuild_count > 0);
        if (!needs_link) {
            time_t bin_mtime = get_mtime(nstr_cstr(output_path));
            if (bin_mtime == 0) {
                needs_link = 1;
            } else {
                for (size_t i = 0; i < all_sources_count; i++) {
                    if (get_mtime(nstr_cstr(obj_paths[i])) > bin_mtime) {
                        needs_link = 1;
                        break;
                    }
                }
            }
        }

        if (!needs_link) {
            log_print(LOG_OK, "Binary up to date: %s\n", nstr_cstr(output_path));
            result = 0;
            goto cleanup;
        }
        // Expand enum flags for linking
        const char *expanded_flags[32];
        size_t exp_count = 0;
        exp_count += sanitizers_to_flags(project->sanitizers, expanded_flags + exp_count, 32 - exp_count);

        // argv: [cc, -o, output, ...expanded, ...objs, NULL]
        size_t argc = 3 + exp_count + all_sources_count;
        char **argv = arena_alloc(arena, sizeof(char *) * (argc + 1));
        if (!argv) goto cleanup;

        size_t a = 0;
        argv[a++] = project->cc;
        argv[a++] = "-o";
        argv[a++] = (char *)nstr_cstr(output_path);
        for (size_t e = 0; e < exp_count; e++)
            argv[a++] = (char *)expanded_flags[e];
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
