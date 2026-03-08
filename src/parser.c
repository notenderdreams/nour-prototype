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
    int  depth    = 0;
    int  in_decl  = 0;

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

            // Already valid C — pass through as-is, just track depth for ';' injection
            fputs(line, out);
            depth   = 1;
            in_decl = 1;

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
