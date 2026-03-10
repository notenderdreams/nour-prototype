#include "parser.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>

// Match "TypeName SymbolName = {" (the only valid nour declaration form).
// Returns 1 and fills type_out/name_out if matched.
static int match_decl(const char *line, char *type_out, char *name_out) {
    const char *p = line;

    // skip leading whitespace
    while (*p && isspace((unsigned char)*p)) p++;

    // first word: type name
    if (!isalpha((unsigned char)*p) && *p != '_') return 0;
    size_t i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < NOUR_DECL_MAX_NAME - 1)
        type_out[i++] = *p++;
    type_out[i] = '\0';

    // must be followed by whitespace
    if (!isspace((unsigned char)*p)) return 0;
    while (*p && isspace((unsigned char)*p)) p++;

    // second word: symbol name
    if (!isalpha((unsigned char)*p) && *p != '_') return 0;
    i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < NOUR_DECL_MAX_NAME - 1)
        name_out[i++] = *p++;
    name_out[i] = '\0';

    // must have '= {'
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '=') return 0;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    return (*p == '{') ? 1 : 0;
}

// Count net brace depth change in a string.
static void count_braces(const char *line, int *depth) {
    for (const char *p = line; *p; p++) {
        if (*p == '{')      (*depth)++;
        else if (*p == '}') (*depth)--;
    }
}

// Detect ".field = {" where '{' is NOT preceded by ')' (no existing cast).
// Returns pointer to the '{', or NULL if not a bare array assignment.
// If field_out is non-NULL, writes the field name into it.
static const char *match_bare_array(const char *line, char *field_out, size_t field_max) {
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '.') return NULL;
    p++;
    // identifier — capture into field_out
    if (!isalpha((unsigned char)*p) && *p != '_') return NULL;
    size_t fi = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
        if (field_out && fi < field_max - 1) field_out[fi++] = *p;
        p++;
    }
    if (field_out) field_out[fi] = '\0';
    // ' = '
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '=') return NULL;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '{') return NULL;
    // make sure previous non-space before '{' wasn't ')' — i.e. not already cast
    const char *look = p - 1;
    while (look > line && isspace((unsigned char)*look)) look--;
    if (*look == ')') return NULL;
    return p; // points at '{'
}

// Returns the compound-literal cast to use for a given field name.
static const char *cast_for_field(const char *field) {
    if (strcmp(field, "targets") == 0) return "(void*[])";
    if (strcmp(field, "deps") == 0)    return "(void*[])";
    return "(char*[])";
}

// Fields whose string values are file paths that need base_dir prefixing.
static int is_path_field(const char *field) {
    return strcmp(field, "sources") == 0
        || strcmp(field, "includes") == 0
        || strcmp(field, "lib") == 0;
}

// Detect ".field = \"value\"" (single-value string assignment, not array).
// Returns 1 if matched and writes field name into field_out.
static int match_single_string_field(const char *line, char *field_out, size_t field_max) {
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '.') return 0;
    p++;
    if (!isalpha((unsigned char)*p) && *p != '_') return 0;
    size_t fi = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
        if (fi < field_max - 1) field_out[fi++] = *p;
        p++;
    }
    field_out[fi] = '\0';
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '=') return 0;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '{') return 0; // that's an array, not single value
    if (*p == '"') return 1; // single string value
    return 0;
}

// Rewrite all "..." strings in a line by prepending base_dir.
// Writes result to out (must be large enough). Skips absolute paths.
static void rewrite_strings_in_line(const char *line, const char *base_dir,
                                    char *out, size_t out_max) {
    size_t oi = 0;
    const char *p = line;
    size_t bd_len = strlen(base_dir);

    while (*p && oi < out_max - 1) {
        if (*p == '"') {
            out[oi++] = *p++; // opening quote
            // If string starts with '/' it's absolute — don't prefix
            if (*p != '/' && bd_len > 0) {
                // Prepend base_dir
                for (size_t j = 0; j < bd_len && oi < out_max - 1; j++)
                    out[oi++] = base_dir[j];
            }
            // Copy until closing quote
            while (*p && *p != '"' && oi < out_max - 1)
                out[oi++] = *p++;
            if (*p == '"' && oi < out_max - 1)
                out[oi++] = *p++; // closing quote
        } else {
            out[oi++] = *p++;
        }
    }
    out[oi] = '\0';
}

// Check if a line is  #include "something.nour"
// If so, write the included filename (between quotes) into path_out.
// Returns 1 on match, 0 otherwise.
static int match_nour_include(const char *line, char *path_out, size_t path_max) {
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '#') return 0;
    p++;
    while (*p && isspace((unsigned char)*p)) p++;
    if (strncmp(p, "include", 7) != 0) return 0;
    p += 7;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p != '"') return 0;
    p++;
    const char *start = p;
    while (*p && *p != '"') p++;
    if (*p != '"') return 0;
    size_t len = (size_t)(p - start);
    if (len < 5 || memcmp(start + len - 5, ".nour", 5) != 0) return 0;
    if (len >= path_max) return 0;
    memcpy(path_out, start, len);
    path_out[len] = '\0';
    return 1;
}

// Resolve an include path relative to the directory of the including file.
static void resolve_include_path(const char *base_file, const char *include_name,
                                 char *out, size_t out_max) {
    if (include_name[0] == '/') {
        snprintf(out, out_max, "%s", include_name);
        return;
    }

    // Make a mutable copy for dirname()
    char base_copy[1024];
    strncpy(base_copy, base_file, sizeof(base_copy) - 1);
    base_copy[sizeof(base_copy) - 1] = '\0';
    const char *dir = dirname(base_copy);

    snprintf(out, out_max, "%s/%s", dir, include_name);
}

// Compute the base directory of a file path (e.g. "sandbox/compiled_static/foo.nour"
// → "sandbox/compiled_static/"). Returns "" for files in the current directory.
static void compute_base_dir(const char *file_path, char *out, size_t out_max) {
    const char *last_slash = strrchr(file_path, '/');
    if (!last_slash) {
        out[0] = '\0';
        return;
    }
    size_t len = (size_t)(last_slash - file_path + 1); // include trailing /
    if (len >= out_max) len = out_max - 1;
    memcpy(out, file_path, len);
    out[len] = '\0';
}

// Internal recursive preprocessor: reads input_path, writes to already-opened out,
// collects declarations, tracks visited files to prevent circular includes.
// base_dir is the directory prefix to prepend to path fields (empty for root file).
static int nour_preprocess_recursive(const char *input_path, const char *base_dir,
                                     FILE *out,
                                     NourDecl *decls, size_t *decl_count,
                                     NourImport *imports, size_t *import_count,
                                     const char **visited, size_t *visited_count) {
    // Circular include guard
    for (size_t i = 0; i < *visited_count; i++) {
        if (strcmp(visited[i], input_path) == 0) {
            log_print(LOG_WARN, "Skipping circular include: %s", input_path);
            return 0;
        }
    }
    if (*visited_count >= NOUR_MAX_INCLUDES) {
        log_print(LOG_ERROR, "Too many nested .nour includes (max %d)", NOUR_MAX_INCLUDES);
        return 1;
    }

    // We need a persistent copy of the path for the visited list.
    // Use a static pool since the recursion depth is bounded.
    static char path_pool[NOUR_MAX_INCLUDES][1024];
    size_t slot = *visited_count;
    strncpy(path_pool[slot], input_path, sizeof(path_pool[slot]) - 1);
    path_pool[slot][sizeof(path_pool[slot]) - 1] = '\0';
    visited[*visited_count] = path_pool[slot];
    (*visited_count)++;

    FILE *in = fopen(input_path, "r");
    if (!in) {
        log_print(LOG_ERROR, "Cannot open: %s", input_path);
        return 1;
    }

    log_print(LOG_INFO, "Preprocessing: %s", input_path);
    if (base_dir[0])
        log_print(LOG_ALIGNED, "base_dir: %s", base_dir);

    char line[1024];
    int  depth       = 0;
    int  in_decl     = 0;
    int  in_array    = 0;
    int  array_depth = 0;
    int  path_field  = 0; // whether the current array field needs path rewriting

    while (fgets(line, sizeof(line), in)) {
        char type[NOUR_DECL_MAX_NAME], sym[NOUR_DECL_MAX_NAME];

        // Check for #include "*.nour" – recursively preprocess and inline
        char inc_name[512];
        if (!in_decl && !in_array && match_nour_include(line, inc_name, sizeof(inc_name))) {
            char resolved[1024];
            resolve_include_path(input_path, inc_name, resolved, sizeof(resolved));

            // Compute the included file's base_dir for path rewriting
            char child_base_dir[NOUR_MAX_PATH];
            compute_base_dir(resolved, child_base_dir, sizeof(child_base_dir));

            // Record this import
            if (*import_count < NOUR_MAX_INCLUDES) {
                strncpy(imports[*import_count].path, resolved, NOUR_MAX_PATH - 1);
                imports[*import_count].path[NOUR_MAX_PATH - 1] = '\0';
                strncpy(imports[*import_count].base_dir, child_base_dir, NOUR_MAX_PATH - 1);
                imports[*import_count].base_dir[NOUR_MAX_PATH - 1] = '\0';
                (*import_count)++;
            }

            fprintf(out, "// --- included from %s ---\n", inc_name);
            int rc = nour_preprocess_recursive(resolved, child_base_dir, out,
                                               decls, decl_count,
                                               imports, import_count,
                                               visited, visited_count);
            fprintf(out, "// --- end %s ---\n", inc_name);
            if (rc != 0) {
                fclose(in);
                return rc;
            }
            continue;
        }

        if (!in_decl && depth == 0 && match_decl(line, type, sym)) {
            // Record declaration
            if (*decl_count < NOUR_MAX_DECLS) {
                strncpy(decls[*decl_count].type, type, NOUR_DECL_MAX_NAME - 1);
                decls[*decl_count].type[NOUR_DECL_MAX_NAME - 1] = '\0';
                strncpy(decls[*decl_count].name, sym,  NOUR_DECL_MAX_NAME - 1);
                decls[*decl_count].name[NOUR_DECL_MAX_NAME - 1] = '\0';
                (*decl_count)++;
            }
            fputs(line, out);
            depth   = 1;
            in_decl = 1;

        } else if (in_decl && !in_array) {
            char field_name[NOUR_DECL_MAX_NAME] = {0};
            const char *brace = match_bare_array(line, field_name, sizeof(field_name));
            if (brace) {
                path_field = (base_dir[0] && is_path_field(field_name));
                const char *cast = cast_for_field(field_name);
                fwrite(line, 1, (size_t)(brace - line), out);
                fputs(cast, out);
                fputs("{", out);

                const char *rest = brace + 1;

                int d = 1;
                for (const char *p = rest; *p; p++) {
                    if (*p == '{')      d++;
                    else if (*p == '}') d--;
                }

                if (d == 0) {
                    // Single-line array — rewrite if path field
                    if (path_field) {
                        char rewritten[2048];
                        // Extract just the content between braces
                        char *last = strrchr((char *)rest, '}');
                        char content[2048];
                        size_t clen = (size_t)(last - rest);
                        if (clen >= sizeof(content)) clen = sizeof(content) - 1;
                        memcpy(content, rest, clen);
                        content[clen] = '\0';
                        rewrite_strings_in_line(content, base_dir, rewritten, sizeof(rewritten));
                        fputs(rewritten, out);
                        fputs(", NULL}", out);
                        fputs(last + 1, out);
                    } else {
                        char *last = strrchr((char *)rest, '}');
                        fwrite(rest, 1, (size_t)(last - rest), out);
                        fputs(", NULL}", out);
                        fputs(last + 1, out);
                    }
                } else {
                    // Multi-line array — rewrite rest if path field
                    if (path_field) {
                        char rewritten[2048];
                        rewrite_strings_in_line(rest, base_dir, rewritten, sizeof(rewritten));
                        fputs(rewritten, out);
                    } else {
                        fputs(rest, out);
                    }
                    in_array    = 1;
                    array_depth = d;
                }
            } else {
                // Check for single-value string path field: .lib = "path"
                char sfield[NOUR_DECL_MAX_NAME] = {0};
                if (base_dir[0] && match_single_string_field(line, sfield, sizeof(sfield))
                    && is_path_field(sfield)) {
                    char rewritten[2048];
                    rewrite_strings_in_line(line, base_dir, rewritten, sizeof(rewritten));
                    fputs(rewritten, out);
                } else {
                    fputs(line, out);
                }
                count_braces(line, &depth);
                if (in_decl && depth == 0)
                    in_decl = 0;
            }

        } else if (in_array) {
            int delta = 0;
            for (const char *p = line; *p; p++) {
                if (*p == '{')      delta++;
                else if (*p == '}') delta--;
            }

            if (array_depth + delta == 0) {
                char *last_brace = strrchr(line, '}');
                if (path_field) {
                    char content[2048], rewritten[2048];
                    size_t clen = (size_t)(last_brace - line);
                    if (clen >= sizeof(content)) clen = sizeof(content) - 1;
                    memcpy(content, line, clen);
                    content[clen] = '\0';
                    rewrite_strings_in_line(content, base_dir, rewritten, sizeof(rewritten));
                    fputs(rewritten, out);
                } else {
                    fwrite(line, 1, (size_t)(last_brace - line), out);
                }
                fputs("NULL\n}", out);
                fputs(last_brace + 1, out);
                array_depth = 0;
                in_array    = 0;
                path_field  = 0;
            } else {
                array_depth += delta;
                if (path_field) {
                    char rewritten[2048];
                    rewrite_strings_in_line(line, base_dir, rewritten, sizeof(rewritten));
                    fputs(rewritten, out);
                } else {
                    fputs(line, out);
                }
            }

        } else {
            count_braces(line, &depth);
            if (in_decl && depth == 0)
                in_decl = 0;
            fputs(line, out);
        }
    }

    fclose(in);
    return 0;
}

int nour_preprocess(const char *input_path, const char *output_path,
                    NourDecl *decls, size_t *decl_count,
                    NourImport *imports, size_t *import_count) {
    *decl_count = 0;
    *import_count = 0;

    FILE *out = fopen(output_path, "w");
    if (!out) {
        log_print(LOG_ERROR, "Cannot open output: %s", output_path);
        return 1;
    }

    const char *visited[NOUR_MAX_INCLUDES];
    size_t visited_count = 0;

    // Root file: base_dir is "" so its paths are NOT rewritten
    int rc = nour_preprocess_recursive(input_path, "", out,
                                       decls, decl_count,
                                       imports, import_count,
                                       visited, &visited_count);
    fclose(out);

    if (rc == 0 && *import_count > 0) {
        log_print(LOG_INFO, "Imported %zu config file(s):", *import_count);
        for (size_t i = 0; i < *import_count; i++)
            log_print(LOG_ALIGNED, "%s (base: %s)",
                      imports[i].path, imports[i].base_dir);
    }

    return rc;
}
