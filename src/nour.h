#ifndef NOUR_H
#define NOUR_H

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

typedef struct {
    OptLevel    optimize;
    Warnings    warnings;
    Sanitizers  sanitizers;
    char      **cflags;
} Profile;

typedef struct {
    char       *version;
    char      **sources;
    char       *cc;
    char       *linker;
    char       *build_dir;
    OptLevel    optimize;
    Warnings    warnings;
    Sanitizers  sanitizers;
    char      **cflags;
} Project;


#endif // NOUR_H
