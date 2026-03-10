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
    // Make a mutable copy for dirname()
    char base_copy[1024];
    strncpy(base_copy, base_file, sizeof(base_copy) - 1);
    base_copy[sizeof(base_copy) - 1] = '\0';
    const char *dir = dirname(base_copy);

    snprintf(out, out_max, "%s/%s", dir, include_name);
}

// Internal recursive preprocessor: reads input_path, writes to already-opened out,
// collects declarations, tracks visited files to prevent circular includes.
static int nour_preprocess_recursive(const char *input_path, FILE *out,
                                     NourDecl *decls, size_t *decl_count,
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

    char line[1024];
    int  depth       = 0;
    int  in_decl     = 0;
    int  in_array    = 0;
    int  array_depth = 0;

    while (fgets(line, sizeof(line), in)) {
        char type[NOUR_DECL_MAX_NAME], sym[NOUR_DECL_MAX_NAME];

        // Check for #include "*.nour" – recursively preprocess and inline
        char inc_name[512];
        if (!in_decl && !in_array && match_nour_include(line, inc_name, sizeof(inc_name))) {
            char resolved[1024];
            resolve_include_path(input_path, inc_name, resolved, sizeof(resolved));

            fprintf(out, "// --- included from %s ---\n", inc_name);
            int rc = nour_preprocess_recursive(resolved, out, decls, decl_count,
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
                    char *last = strrchr((char *)rest, '}');
                    fwrite(rest, 1, (size_t)(last - rest), out);
                    fputs(", NULL}", out);
                    fputs(last + 1, out);
                } else {
                    fputs(rest, out);
                    in_array    = 1;
                    array_depth = d;
                }
            } else {
                count_braces(line, &depth);
                if (in_decl && depth == 0)
                    in_decl = 0;
                fputs(line, out);
            }

        } else if (in_array) {
            int delta = 0;
            for (const char *p = line; *p; p++) {
                if (*p == '{')      delta++;
                else if (*p == '}') delta--;
            }

            if (array_depth + delta == 0) {
                char *last_brace = strrchr(line, '}');
                fwrite(line, 1, (size_t)(last_brace - line), out);
                fputs("NULL\n}", out);
                fputs(last_brace + 1, out);
                array_depth = 0;
                in_array    = 0;
            } else {
                array_depth += delta;
                fputs(line, out);
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
                    NourDecl *decls, size_t *decl_count) {
    *decl_count = 0;

    FILE *out = fopen(output_path, "w");
    if (!out) {
        log_print(LOG_ERROR, "Cannot open output: %s", output_path);
        return 1;
    }

    const char *visited[NOUR_MAX_INCLUDES];
    size_t visited_count = 0;

    int rc = nour_preprocess_recursive(input_path, out, decls, decl_count,
                                       visited, &visited_count);
    fclose(out);
    return rc;
}
