#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>

#define NOUR_DECL_MAX_NAME 64
#define NOUR_MAX_DECLS     64
#define NOUR_MAX_INCLUDES  16
#define NOUR_MAX_PATH      1024

typedef struct {
    char type[NOUR_DECL_MAX_NAME]; // e.g. "Project", "Module"
    char name[NOUR_DECL_MAX_NAME]; // e.g. "test_build", "math_utils"
} NourDecl;

// Tracks each imported .nour file and its resolved base directory.
// The base_dir is used to rewrite relative paths in path fields
// (sources, includes, lib) so they resolve from the project root.
typedef struct {
    char path[NOUR_MAX_PATH];      // resolved path of the .nour file
    char base_dir[NOUR_MAX_PATH];  // directory prefix for path rewriting
} NourImport;

// Preprocess a .nour file (and any #include'd .nour files) into valid C.
// Included .nour files are recursively preprocessed and inlined.
// Path fields in included files are rewritten relative to the project root.
// Fills decls[] with all found declarations across all files, sets *decl_count.
// Fills imports[] with imported file info, sets *import_count.
// Returns 0 on success, non-zero on error.
int nour_preprocess(
    const char *input_path,
    const char *output_path,
    NourDecl   *decls,
    size_t     *decl_count,
    NourImport *imports,
    size_t     *import_count
);

#endif // PARSER_H
