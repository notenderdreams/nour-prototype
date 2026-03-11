#ifndef NOUR_H
#define NOUR_H

// ── Target discrimination ───────────────────────────────────────────
// The first field of every target struct is `TargetKind kind;`.
// Given a void* from the targets array you can always inspect
// *(TargetKind*)ptr to decide which concrete struct it points to.

typedef enum {
    TARGET_EXECUTABLE,
    TARGET_LIBRARY,
    TARGET_PACKAGE,
} TargetKind;

// ── Library flavour ─────────────────────────────────────────────────

typedef enum {
    STATIC,  // static library  (.a)
    SHARED,  // shared library  (.so / .dylib)
} LibraryType;

// ── Legacy project-level type (kept for backward compat) ────────────

typedef enum {
    EXE,         // executable (default)
    STATIC_LIB,  // static library  (.a)
    DYNAMIC_LIB  // shared library  (.so / .dylib)
} ProjectType;

// ── Compiler / build flags ──────────────────────────────────────────

typedef enum {
    OPT_NONE,       // -O0
    OPT_DEBUG,      // -Og
    OPT_RELEASE,    // -O2
    OPT_SIZE,       // -Os
    OPT_AGGRESSIVE  // -O3
} OptLevel;

typedef enum {
    WARN_NONE      = 0,
    WARN_ALL       = 1 << 0,  // -Wall
    WARN_EXTRA     = 1 << 1,  // -Wextra
    WARN_ERROR     = 1 << 2,  // -Werror
    WARN_PEDANTIC  = 1 << 3   // -Wpedantic
} Warnings;

typedef enum {
    SAN_NONE    = 0,
    SAN_ADDRESS = 1 << 0,  // -fsanitize=address
    SAN_UB      = 1 << 1,  // -fsanitize=undefined
    SAN_THREAD  = 1 << 2,  // -fsanitize=thread
    SAN_MEMORY  = 1 << 3   // -fsanitize=memory
} Sanitizers;

// ── Target structs ──────────────────────────────────────────────────

typedef struct {
    TargetKind   kind;      // TARGET_EXECUTABLE (set automatically)
    char        *name;      // symbol name (set automatically)
    char       **sources;
    char       **includes;
    void       **deps;      // NULL-terminated list of Executable*/Library*
} Executable;

typedef struct {
    TargetKind   kind;      // TARGET_LIBRARY (set automatically)
    char        *name;      // symbol name (set automatically)
    LibraryType  type;      // STATIC or SHARED
    char       **sources;
    char       **includes;
    void       **deps;      // NULL-terminated list of Library*
} Library;

// ── Pre-compiled package ────────────────────────────────────────────
// A Package is an already-compiled library set (.a / .so / .dylib).
// sources contains one or more prebuilt library paths.

typedef struct {
    TargetKind   kind;      // TARGET_PACKAGE (set automatically)
    char        *name;      // symbol name (set automatically)
    char       **sources;   // prebuilt lib paths (.a / .so / .dylib)
    char       **includes;  // header search paths
} Package;

// ── Build profile ───────────────────────────────────────────────────

typedef struct {
    OptLevel    optimize;
    Warnings    warnings;
    Sanitizers  sanitizers;
    char      **cflags;
} Profile;

// ── Project ─────────────────────────────────────────────────────────

typedef struct {
    char       *version;
    char       *cc;
    char       *linker;
    char       *build_dir;
    OptLevel    optimize;
    Warnings    warnings;
    Sanitizers  sanitizers;
    char      **cflags;
    void      **targets;    // NULL-terminated array of Executable*/Library*/Package*

    // Legacy fields – kept so the old compile path still works.
    char      **sources;
    ProjectType type;
} Project;


#endif // NOUR_H
