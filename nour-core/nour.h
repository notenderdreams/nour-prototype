#ifndef NOUR_H
#define NOUR_H

// ── Target discrimination ────────────────────────────────────────────
// First field of every target struct.
// Cast any void* from deps[] to TargetKind* to dispatch.

typedef enum {
    TARGET_EXECUTABLE,
    TARGET_LIBRARY,
    TARGET_PACKAGE,
} TargetKind;

// ── Library types  ───────────────────────────────────────────────────

typedef enum {
    STATIC,  // .a
    SHARED,  // .so / .dylib
} LibraryType;

// ── Compiler flags ───────────────────────────────────────────────────

typedef enum {
    OPT_NONE,        // -O0
    OPT_DEBUG,       // -Og
    OPT_RELEASE,     // -O2
    OPT_SIZE,        // -Os
    OPT_AGGRESSIVE,  // -O3
} OptLevel;

typedef enum {
    WARN_NONE     = 0,
    WARN_ALL      = 1 << 0,  // -Wall
    WARN_EXTRA    = 1 << 1,  // -Wextra
    WARN_ERROR    = 1 << 2,  // -Werror
    WARN_PEDANTIC = 1 << 3,  // -Wpedantic
} Warnings;

typedef enum {
    SAN_NONE    = 0,
    SAN_ADDRESS = 1 << 0,  // -fsanitize=address
    SAN_UB      = 1 << 1,  // -fsanitize=undefined
    SAN_THREAD  = 1 << 2,  // -fsanitize=thread
    SAN_MEMORY  = 1 << 3,  // -fsanitize=memory
} Sanitizers;

// ── Target structs ───────────────────────────────────────────────────
// No name field, symbol name tracked by parser/loader externally.

typedef struct {
    TargetKind   kind;      // TARGET_EXECUTABLE
    char       **sources;
    char       **includes;
    void       **deps;      // NULL-terminated: Executable* | Library* | Package*
} Executable;

typedef struct {
    TargetKind   kind;      // TARGET_LIBRARY
    LibraryType  type;      // STATIC or SHARED
    char       **sources;
    char       **includes;
    void       **deps;      // NULL-terminated: Library* | Package*
} Library;

typedef struct {
    TargetKind   kind;      // TARGET_PACKAGE
    char       **sources;   // prebuilt lib paths (.a / .so)
    char       **includes;  // header search paths
} Package;

// ── Project ──────────────────────────────────────────────────────────

typedef struct {
    char *version;
    char *cc;       // "gcc" | "clang"
    char *linker;   // NULL → cc default
} Project;

// ── Profile ──────────────────────────────────────────────────────────
// build_dir derived as build/<profile_name> at compile time.

typedef struct {
    OptLevel    optimize;
    Warnings    warnings;
    Sanitizers  sanitizers;
    char      **cflags;     // NULL-terminated extra flags
} Profile;

#endif // NOUR_H
