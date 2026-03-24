/*
 * preprocess.h
 * ------------
 * .nour file preprocessor
 *
 * Transforms a .nour config file into a valid c source by injecting
 * compound-literal casts, and NULL sentinels into array fields, rewriting
 * relative paths in included files and inlining #include'd .nour files
 * recursively. Also collects every declaration :
 *      - Project
 *      - Profile
 *      - Executable, Library, Package
 * so loader knows which symbols to dlsym from the compiled .so
 */

#ifndef PREPROCESS_H
#define PREPROCESS_H

#include "types.h"
#include <limits.h>

#define NOUR_DECL_LIMIT    64   /* max declarations per .nour file        */
#define NOUR_IDENT_LEN     64   /* max length of a type or symbol name    */
#define NOUR_IMPORT_LIMIT  16   /* max #include'd .nour files per project */


/*
 * [Nour Declaration]
 * Collection of all the declarations found across a .nour file and it's
 * included files. Keeps the Declaration Types (NourDecType), their names
 * and count.
 */
typedef enum {
    DECL_PROJECT,
    DECL_PROFILE,
    DECL_EXECUTABLE,
    DECL_LIBRARY,
    DECL_PACKAGE,
} NourDeclType;

typedef struct {
    NourDeclType type;
    char         name[NOUR_IDENT_LEN];
} NourDecl;


typedef struct {
    NourDecl decls[NOUR_DECL_LIMIT];
    u64      count;
} NourDecls;

int nour_preprocess(const char *input_path, const char *output_path, NourDecls *out);

#endif /* PREPROCESS_H */