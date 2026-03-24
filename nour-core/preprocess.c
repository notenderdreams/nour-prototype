#include "preprocess.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <limits.h>

static NourDeclType decl_type_from_str(const char *s) {
    if (!strcmp(s, "Project"))    return DECL_PROJECT;
    if (!strcmp(s, "Profile"))    return DECL_PROFILE;
    if (!strcmp(s, "Executable")) return DECL_EXECUTABLE;
    if (!strcmp(s, "Library"))    return DECL_LIBRARY;
    if (!strcmp(s, "Package"))    return DECL_PACKAGE;
    return (NourDeclType)-1;
}

static const char *skip_spaces(const char *p) {
    while (*p && isspace((unsigned char)*p))
        ++p;
    return p;
}

/*
 * Scans a single line for a nour declaration of the form:
 *      TypeName SymbolName = {
 * On match writes into `type_out` & `name_out` and returns 1.
 * Returns 0 on anything else.
 */
static int match_decl(const char *line, char *type_out, char *name_out) {
    const char *p = line;
    p = skip_spaces(p);

    if (!isalpha((unsigned char)*p) && *p != '_')
        return 0;

    u64 i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < NOUR_IDENT_LEN - 1)
        type_out[i++] = *p++;
    type_out[i] = '\0';

    if (!isspace((unsigned char)*p))
        return 0;

    p = skip_spaces(p);
    if (!isalpha((unsigned char)*p) && *p != '_')
        return 0;

    i = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < NOUR_IDENT_LEN - 1)
        name_out[i++] = *p++;
    name_out[i] = '\0';

    p = skip_spaces(p);
    if (*p != '=') return 0;
    ++p;
    p = skip_spaces(p);
    return (*p == '{') ? 1 : 0;
}

/* Used to track when a declaration block closes (depth hits 0) */
static void count_braces(const char *line, i32 *depth) {
    for (const char *p = line; *p; ++p) {
        if      (*p == '{') ++(*depth);
        else if (*p == '}') --(*depth);
    }
}

/*
 * Detects a bare array assignment of the form:
 *      .fieldname = {
 * where the '{' has no preceding cast like '(char*[])'.
 * Writes the name into `field_out` and returns a pointer to the '{'
 * in the original line.
 * Returns NULL if the line doesn't match or is already cast.
 */
static const char *match_bare_array(const char *line, char *field_out, u64 field_max) {
    const char *p = line;
    p = skip_spaces(p);
    if (*p != '.') return NULL;

    ++p;
    if (!isalpha((unsigned char)*p) && *p != '_') return NULL;

    u64 fi = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
        if (fi < field_max - 1) field_out[fi++] = *p;
        ++p;
    }
    field_out[fi] = '\0';

    p = skip_spaces(p);
    if (*p != '=') return NULL;
    ++p;
    p = skip_spaces(p);
    if (*p != '{') return NULL;

    /* already cast if previous non-space char was ')' */
    const char *look = p - 1;
    while (look > line && isspace((unsigned char)*look)) --look;
    if (*look == ')') return NULL;

    return p;
}

/* Returns appropriate cast string for a field */
static const char *cast_for_field(const char *field) {
    if (!strcmp(field, "deps")) return "(void*[])";
    return "(char*[])";
}

/* Returns 1 if the field holds file paths that need to be rewritten with base_dir */
static int is_path_field(const char *field) {
    return !strcmp(field, "sources")
        || !strcmp(field, "includes");
}

/*
 * Detects a single value string assignment of the form:
 *      .fieldname = "value"
 * as opposed to an array assignment (.field = {...}).
 * Used to catch path fields that aren't arrays so their
 * string value can still be rewritten with base_dir.
 * Returns 1 on match, writes field name into field_out.
 */
static int match_single_string_field(const char *line, char *field_out, u64 field_max) {
    const char *p = line;
    p = skip_spaces(p);
    if (*p != '.') return 0;

    ++p;
    if (!isalpha((unsigned char)*p) && *p != '_') return 0;

    u64 fi = 0;
    while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
        if (fi < field_max - 1) field_out[fi++] = *p;
        ++p;
    }
    field_out[fi] = '\0';

    p = skip_spaces(p);
    if (*p != '=') return 0;
    ++p;
    p = skip_spaces(p);
    if (*p == '{') return 0;
    return (*p == '"') ? 1 : 0;
}

/*
 * Extracts the directory portion of a file path including the
 * trailing slash.
 * e.g. "sandbox/libs/math.nour" -> "sandbox/libs/"
 *      "project.nour"           -> ""
 * Used to build the base_dir prefix for path rewriting in .nour files.
 */
static void get_base_dir(const char *file_path, char *out, u64 out_max) {
    const char *last_slash = strrchr(file_path, '/');
    if (!last_slash) { out[0] = '\0'; return; }
    u64 len = (u64)(last_slash - file_path + 1);
    if (len >= out_max) len = out_max - 1;
    memcpy(out, file_path, len);
    out[len] = '\0';
}

/*
 * Returns 1 if the line is a #include directive pointing at a
 * .nour file (e.g. #include "libs/math.nour").
 * Writes the quoted filename into path_out on match.
 * .h files are skipped.
 */
static int match_nour_include(const char *line, char *path_out, u64 path_max) {
    const char *p = line;
    p = skip_spaces(p);
    if (*p != '#') return 0;

    ++p;
    p = skip_spaces(p);
    if (strncmp(p, "include", 7)) return 0;
    p += 7;
    p = skip_spaces(p);
    if (*p != '"') return 0;
    ++p;

    const char *start = p;
    while (*p && *p != '"') ++p;
    if (*p != '"') return 0;

    u64 len = (u64)(p - start);
    if (len < 5 || memcmp(start + len - 5, ".nour", 5)) return 0;
    if (len >= path_max) return 0;
    memcpy(path_out, start, len);
    path_out[len] = '\0';
    return 1;
}

/*
 * Rewrite quoted path literals in a line relative to `base_dir`.
 * If the string begins with '/', it is treated as absolute and
 * copied unchanged. Otherwise base_dir is prepended.
 * A literal "." collapses from `base_dir + "/."` to `base_dir`.
 * Writes the result into out[0..out_max].
 */
static void resolve_relative_paths_in_line(const char *line, const char *base_dir,
                                            char *out, u64 out_max) {
    u64        oi     = 0;
    u64        bd_len = strlen(base_dir);
    const char *p     = line;

    while (*p && oi < out_max - 1) {
        if (*p == '"') {
            out[oi++] = *p++;
            if (*p != '/' && bd_len > 0) {
                for (u64 j = 0; j < bd_len && oi < out_max - 1; ++j)
                    out[oi++] = base_dir[j];
            }
            const char *val_start = out + oi;
            while (*p && *p != '"' && oi < out_max - 1)
                out[oi++] = *p++;

            /* collapse base_dir + "." -> base_dir (strip trailing slash + dot) */
            u64 written = (u64)(out + oi - val_start);
            if (written == 1        &&
                val_start[0] == '.' &&
                bd_len > 0          &&
                base_dir[bd_len - 1] == '/')
                oi -= 2;

            if (*p == '"' && oi < out_max - 1)
                out[oi++] = *p++;
        } else {
            out[oi++] = *p++;
        }
    }
    out[oi] = '\0';
}

/*
 * Builds the full path of an included .nour file relative to the
 * directory of the file that contains the #include directive.
 * e.g. base_file="sandbox/project.nour", include_name="libs/math.nour"
 *   -> out = "sandbox/libs/math.nour"
 * Absolute include paths are passed through as-is.
 */
static void resolve_include_path(const char *base_file, const char *include_name,
                                  char *out, u64 out_max) {
    if (include_name[0] == '/') {
        snprintf(out, out_max, "%s", include_name);
        return;
    }
    char base_copy[PATH_MAX];
    strncpy(base_copy, base_file, sizeof(base_copy) - 1);
    base_copy[sizeof(base_copy) - 1] = '\0';
    const char *dir = dirname(base_copy);
    snprintf(out, out_max, "%s/%s", dir, include_name);
}

/*
 * [Recursive preprocessor]
 * Core preprocessing loop. Reads input_path line by line and writes
 * transformed output to the already open FILE *out.
 *
 * What it does per line:
 *   - #include "*.nour"  -> recursively inlines the included file
 *   - TypeName Sym = {   -> records the declaration in decls, writes as-is
 *   - .field = {         -> injects the correct cast + NULL sentinel
 *   - path field strings -> rewrites relative paths with base_dir prefix
 *   - everything else    -> written through unchanged
 */
static int preprocess_recursive(const char *input_path, const char *base_dir,
                                 FILE *out, Vector *decls,
                                 const char **visited, u64 *visited_count) {
    for (u64 i = 0; i < *visited_count; ++i)
        if (!strcmp(visited[i], input_path)) return 0;

    if (*visited_count >= NOUR_IMPORT_LIMIT) {
        fprintf(stderr, "error: too many nested .nour includes (max %d)\n", NOUR_IMPORT_LIMIT);
        return 1;
    }

    static char path_pool[NOUR_IMPORT_LIMIT][PATH_MAX];
    u64 slot = *visited_count;
    strncpy(path_pool[slot], input_path, PATH_MAX - 1);
    path_pool[slot][PATH_MAX - 1] = '\0';
    visited[*visited_count] = path_pool[slot];
    ++(*visited_count);

    FILE *in = fopen(input_path, "r");
    if (!in) {
        fprintf(stderr, "error: cannot open: %s\n", input_path);
        return 1;
    }

    char line[1024];
    i32  depth      = 0;
    i32  in_decl    = 0;
    i32  in_arr     = 0;
    i32  arr_depth  = 0;
    i32  path_field = 0;

    while (fgets(line, sizeof(line), in)) {
        char type_str[NOUR_IDENT_LEN], sym[NOUR_IDENT_LEN];

        /* handle #include "*.nour" */
        char inc_name[PATH_MAX];
        if (!in_decl && !in_arr
                && match_nour_include(line, inc_name, sizeof(inc_name))) {
            char resolved[PATH_MAX];
            resolve_include_path(input_path, inc_name, resolved, sizeof(resolved));
            char child_base[PATH_MAX];
            get_base_dir(resolved, child_base, sizeof(child_base));

            fprintf(out, "// --- included from %s ---\n", inc_name);
            int rc = preprocess_recursive(resolved, child_base, out,
                                          decls, visited, visited_count);
            fprintf(out, "// --- end %s ---\n", inc_name);
            if (rc != 0) { fclose(in); return rc; }
            continue;
        }

        /* top-level declaration */
        if (!in_decl && depth == 0 && match_decl(line, type_str, sym)) {
            NourDeclType dtype = decl_type_from_str(type_str);
            if (dtype != (NourDeclType)-1) {
                NourDecl *d = malloc(sizeof(NourDecl));
                if (!d) {
                    fprintf(stderr, "error: out of memory\n");
                    fclose(in);
                    return 1;
                }
                d->type = dtype;
                strncpy(d->name, sym, NOUR_IDENT_LEN - 1);
                d->name[NOUR_IDENT_LEN - 1] = '\0';
                vec_push(*decls, d);
            }
            fputs(line, out);
            depth   = 1;
            in_decl = 1;

        } else if (in_decl && !in_arr) {
            char field_name[NOUR_IDENT_LEN] = {0};
            const char *brace = match_bare_array(line, field_name, sizeof(field_name));

            if (brace) {
                path_field       = (base_dir[0] && is_path_field(field_name));
                const char *cast = cast_for_field(field_name);
                fwrite(line, 1, (u64)(brace - line), out);
                fputs(cast, out);
                fputs("{", out);

                const char *rest = brace + 1;
                i32 d = 1;
                for (const char *p = rest; *p; ++p) {
                    if      (*p == '{') ++d;
                    else if (*p == '}') --d;
                }

                if (d == 0) {
                    /* single-line array */
                    char *last = strrchr((char *)rest, '}');
                    if (path_field) {
                        char content[2048], rewritten[2048];
                        u64 clen = (u64)(last - rest);
                        if (clen >= sizeof(content)) clen = sizeof(content) - 1;
                        memcpy(content, rest, clen);
                        content[clen] = '\0';
                        resolve_relative_paths_in_line(content, base_dir,
                                                       rewritten, sizeof(rewritten));
                        fputs(rewritten, out);
                    } else {
                        fwrite(rest, 1, (u64)(last - rest), out);
                    }
                    fputs(", NULL}", out);
                    fputs(last + 1, out);
                    path_field = 0;
                } else {
                    /* multi-line array */
                    if (path_field) {
                        char rewritten[2048];
                        resolve_relative_paths_in_line(rest, base_dir,
                                                       rewritten, sizeof(rewritten));
                        fputs(rewritten, out);
                    } else {
                        fputs(rest, out);
                    }
                    in_arr    = 1;
                    arr_depth = d;
                }

            } else {
                /* single-value string path field */
                char sfield[NOUR_IDENT_LEN] = {0};
                if (base_dir[0]
                        && match_single_string_field(line, sfield, sizeof(sfield))
                        && is_path_field(sfield)) {
                    char rewritten[2048];
                    resolve_relative_paths_in_line(line, base_dir,
                                                   rewritten, sizeof(rewritten));
                    fputs(rewritten, out);
                } else {
                    fputs(line, out);
                }
                count_braces(line, &depth);
                if (in_decl && depth == 0) in_decl = 0;
            }

        } else if (in_arr) {
            i32 delta = 0;
            for (const char *p = line; *p; ++p) {
                if      (*p == '{') ++delta;
                else if (*p == '}') --delta;
            }

            if (arr_depth + delta == 0) {
                char *last_brace = strrchr(line, '}');
                if (path_field) {
                    char content[2048], rewritten[2048];
                    u64 clen = (u64)(last_brace - line);
                    if (clen >= sizeof(content)) clen = sizeof(content) - 1;
                    memcpy(content, line, clen);
                    content[clen] = '\0';
                    resolve_relative_paths_in_line(content, base_dir,
                                                   rewritten, sizeof(rewritten));
                    fputs(rewritten, out);
                } else {
                    fwrite(line, 1, (u64)(last_brace - line), out);
                }
                fputs("NULL\n}", out);
                fputs(last_brace + 1, out);
                arr_depth  = 0;
                in_arr     = 0;
                path_field = 0;
            } else {
                arr_depth += delta;
                if (path_field) {
                    char rewritten[2048];
                    resolve_relative_paths_in_line(line, base_dir,
                                                   rewritten, sizeof(rewritten));
                    fputs(rewritten, out);
                } else {
                    fputs(line, out);
                }
            }

        } else {
            count_braces(line, &depth);
            if (in_decl && depth == 0) in_decl = 0;
            fputs(line, out);
        }
    }

    fclose(in);
    return 0;
}

/*
 * [Public entry point]
 * Opens output_path for writing, computes the root base_dir from
 * input_path, then kicks off preprocess_recursive.
 * On success, decls contains every NourDecl* found across the file
 * and all its #include'd .nour files, heap-allocated and pushed.
 * Caller owns cleanup: free each element then vec_destroy.
 * output_path holds the transformed valid-C source ready to be
 * compiled into a .so.
 * Returns 0 on success, 1 on any error.
 */
int nour_preprocess(const char *input_path, const char *output_path, Vector *decls) {
    FILE *f = fopen(output_path, "w");
    if (!f) {
        fprintf(stderr, "error: cannot open output: %s\n", output_path);
        return 1;
    }

    const char *visited[NOUR_IMPORT_LIMIT];
    u64 visited_count = 0;

    char base_dir[PATH_MAX];
    get_base_dir(input_path, base_dir, sizeof(base_dir));

    int rc = preprocess_recursive(input_path, base_dir, f,
                                  decls, visited, &visited_count);
    fclose(f);
    return rc;
}