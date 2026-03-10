#include "parser.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
    return "(char*[])";
}

int nour_preprocess(const char *input_path, const char *output_path,
                    NourDecl *decls, size_t *decl_count) {
    *decl_count = 0;

    FILE *in = fopen(input_path, "r");
    if (!in) {
        log_print(LOG_ERROR, "Cannot open: %s\n", input_path);
        return 1;
    }

    FILE *out = fopen(output_path, "w");
    if (!out) {
        log_print(LOG_ERROR, "Cannot open: %s\n", output_path);
        fclose(in);
        return 1;
    }

    char line[1024];
    int  depth       = 0;
    int  in_decl     = 0;
    int  in_array    = 0;  // rewriting a bare { } array field
    int  array_depth = 0;

    while (fgets(line, sizeof(line), in)) {
        char type[NOUR_DECL_MAX_NAME], sym[NOUR_DECL_MAX_NAME];

        if (!in_decl && depth == 0 && match_decl(line, type, sym)) {
            // Record declaration
            if (*decl_count < 16) {
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
                // Write ".field = (<cast>){" prefix
                fwrite(line, 1, (size_t)(brace - line), out);
                fputs(cast, out);
                fputs("{", out);

                const char *rest = brace + 1;

                // Count depth in the rest of this line to see if it closes here
                int d = 1;
                for (const char *p = rest; *p; p++) {
                    if (*p == '{')      d++;
                    else if (*p == '}') d--;
                }

                if (d == 0) {
                    // Single-line: inject NULL before the closing '}'
                    char *last = strrchr((char *)rest, '}');
                    fwrite(rest, 1, (size_t)(last - rest), out);
                    fputs(", NULL}", out);
                    fputs(last + 1, out);
                } else {
                    // Multi-line: write rest as-is and enter array mode
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
            // Track depth change this line will cause
            int delta = 0;
            for (const char *p = line; *p; p++) {
                if (*p == '{')      delta++;
                else if (*p == '}') delta--;
            }

            if (array_depth + delta == 0) {
                // This line closes the array — inject NULL before the last '}'
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
    fclose(out);
    return 0;
}
