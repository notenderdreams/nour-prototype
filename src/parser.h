#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#define NOUR_DECL_MAX_NAME 64
#define NOUR_MAX_DECLS     64
#define NOUR_MAX_INCLUDES  16

typedef struct {
    char type[NOUR_DECL_MAX_NAME]; // e.g. "Project", "Module"
    char name[NOUR_DECL_MAX_NAME]; // e.g. "test_build", "math_utils"
} NourDecl;

// Preprocess a .nour file (and any #include'd .nour files) into valid C.
// Included .nour files are recursively preprocessed and inlined.
// Fills decls[] with all found declarations across all files, sets *decl_count.
// Returns 0 on success, non-zero on error.
int nour_preprocess(
    const char *input_path,
    const char *output_path,
    NourDecl   *decls,
    size_t     *decl_count
);

#endif // PARSER_H
