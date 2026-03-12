#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include "nour.h"
#include "log.h"

static inline const char *library_type_str(LibraryType t) {
    switch (t) {
        case STATIC: return "static";
        case SHARED: return "shared";
        default:         return "unknown";
    }
}

static inline size_t count_sources(char **sources) {
    size_t n = 0;
    if (sources) { while (sources[n]) n++; }
    return n;
}

static inline void build_deps_str(void **deps, char *buf, size_t sz) {
    if (!deps || !*deps) { buf[0] = '\0'; return; }
    size_t used = 0;
    /* "  → " in UTF-8 */
    const char arrow[] = "  \xe2\x86\x92 ";
    size_t alen = sizeof(arrow) - 1;
    if (used + alen < sz) { memcpy(buf + used, arrow, alen); used += alen; }
    for (void **d = deps; *d; d++) {
        TargetKind dk = *(TargetKind *)(*d);
        const char *n = NULL;
        if      (dk == TARGET_LIBRARY)    n = ((Library    *)(*d))->name;
        else if (dk == TARGET_EXECUTABLE) n = ((Executable *)(*d))->name;
        else if (dk == TARGET_PACKAGE)    n = ((Package    *)(*d))->name;
        if (!n) n = "?";
        if (d != deps) {
            if (used + 2 < sz) { buf[used++] = ','; buf[used++] = ' '; }
        }
        size_t nl = strlen(n);
        if (used + nl < sz) { memcpy(buf + used, n, nl); used += nl; }
    }
    buf[used < sz ? used : sz - 1] = '\0';
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
    if (exe->deps) {
        log_print(LOG_ALIGNED, "deps:");
        for (void **d = exe->deps; *d; d++) {
            TargetKind dk = *(TargetKind *)(*d);
            if (dk == TARGET_LIBRARY) {
                const Library *dep = (const Library *)(*d);
                log_print(LOG_ALIGNED, "  %s (library)", dep->name ? dep->name : "?");
            } else if (dk == TARGET_EXECUTABLE) {
                const Executable *dep = (const Executable *)(*d);
                log_print(LOG_ALIGNED, "  %s (executable)", dep->name ? dep->name : "?");
            } else if (dk == TARGET_PACKAGE) {
                const Package *dep = (const Package *)(*d);
                log_print(LOG_ALIGNED, "  %s (package)", dep->name ? dep->name : "?");
            }
        }
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
    if (lib->deps) {
        log_print(LOG_ALIGNED, "deps:");
        for (void **d = lib->deps; *d; d++) {
            TargetKind dk = *(TargetKind *)(*d);
            if (dk == TARGET_LIBRARY) {
                const Library *dep = (const Library *)(*d);
                log_print(LOG_ALIGNED, "  %s (library)", dep->name ? dep->name : "?");
            } else if (dk == TARGET_PACKAGE) {
                const Package *dep = (const Package *)(*d);
                log_print(LOG_ALIGNED, "  %s (package)", dep->name ? dep->name : "?");
            }
        }
    }
}

static inline void print_package(const Package *pkg, const char *name) {
    if (!pkg) return;
    const char *n = pkg->name ? pkg->name : (name ? name : "?");
    log_print(LOG_INFO, "Package: %s", n);
    if (pkg->sources) {
        log_print(LOG_ALIGNED, "sources:");
        for (char **s = pkg->sources; *s; s++)
            log_print(LOG_ALIGNED, "  %s", *s);
    }
    if (pkg->includes) {
        log_print(LOG_ALIGNED, "includes:");
        for (char **s = pkg->includes; *s; s++)
            log_print(LOG_ALIGNED, "  %s", *s);
    }
}

static inline void print_target(const void *target, const char *name) {
    if (!target) return;
    TargetKind kind = *(const TargetKind *)target;
    switch (kind) {
        case TARGET_EXECUTABLE: print_executable((const Executable *)target, name); break;
        case TARGET_LIBRARY:    print_library((const Library *)target, name);       break;
        case TARGET_PACKAGE:    print_package((const Package *)target, name);       break;
    }
}

static inline void print_project(const Project *project, const char *name,
                                  const char *project_file) {
    if (!project) return;

    /* ── Console header ─────────────────────────────────────── */
    const char *version  = project->version ? project->version : "?";
    const char *file_str = project_file ? project_file : "";
    int color = console_use_color();
    if (color)
        console_out("\n" COLOR_BOLD "nour v%s" COLOR_RESET
                    "  " COLOR_DIM "%s" COLOR_RESET "\n\n", version, file_str);
    else
        console_out("\nnour v%s  %s\n\n", version, file_str);

    /* ── Console target plan table ──────────────────────────── */
    if (project->targets) {
        for (void **t = project->targets; *t; t++) {
            TargetKind kind = *(TargetKind *)(*t);
            const char *sym, *sym_color, *tgt_name, *kind_str;
            size_t src_count;
            void **deps = NULL;

            if (kind == TARGET_LIBRARY) {
                Library *lib = (Library *)(*t);
                sym       = "\xe2\x88\xb7";  /* ∷ U+2237 */
                sym_color = COLOR_CYAN;
                tgt_name  = lib->name ? lib->name : "?";
                kind_str  = library_type_str(lib->type);
                src_count = count_sources(lib->sources);
                deps      = lib->deps;
            } else if (kind == TARGET_EXECUTABLE) {
                Executable *exe = (Executable *)(*t);
                sym       = "\xe2\x97\x86";  /* ◆ U+25C6 */
                sym_color = COLOR_YELLOW;
                tgt_name  = exe->name ? exe->name : "?";
                kind_str  = "exe";
                src_count = count_sources(exe->sources);
                deps      = exe->deps;
            } else {
                continue;
            }

            char deps_str[256] = "";
            build_deps_str(deps, deps_str, sizeof(deps_str));

            if (color)
                console_out("  %s%s%s %-20s" COLOR_DIM " %-8s %zu sources%s"
                            COLOR_RESET "\n",
                            sym_color, sym, COLOR_RESET,
                            tgt_name, kind_str, src_count, deps_str);
            else
                console_out("  %s %-20s %-8s %zu sources%s\n",
                            sym, tgt_name, kind_str, src_count, deps_str);
        }
        console_out("\n");
    }

    /* ── Verbose log (goes to log file) ────────────────────────── */
    log_print(LOG_INFO, "Project %s v%s (%s)",
              name ? name : "?",
              project->version ? project->version : "?",
              project->cc ? project->cc : "gcc");
    if (project->linker && project->linker[0])
        log_print(LOG_ALIGNED, "linker: %s", project->linker);
    if (project->cflags != NULL) {
        log_print(LOG_ALIGNED, "cflags:");
        for (char **flag = project->cflags; *flag != NULL; flag++)
            log_print(LOG_ALIGNED, "  %s", *flag);
    }
    if (project->targets != NULL) {
        log_print(LOG_ALIGNED, "targets:");
        for (void **t = project->targets; *t; t++)
            print_target(*t, NULL);
    }
}

#endif // UTILS_H