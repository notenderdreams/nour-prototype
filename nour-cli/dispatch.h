#ifndef DISPATCH_H
#define DISPATCH_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

#define DISPATCH_MAX_COMMANDS   32
#define DISPATCH_MAX_FLAGS      32
#define MAX_ARGS                64

#define FLAG_END    { .name = NULL }
#define CMD_END     NULL
#define NO_SHORT '\0'

typedef enum { 
    FLAG_BOOL, 
    FLAG_STR, 
    FLAG_INT, 
    FLAG_FLOAT 
} FlagType;
 
typedef struct {
    const char *name;
    char        shorthand;
    FlagType    type;
    union {
        bool        b;
        const char *s;
        i32         i;
        f32         f;
    } val;
    const char *usage;
    bool        required;
    bool        _set;
} Flag;


typedef struct Command  Command;
typedef struct App      App;

typedef struct {
    Command     *cmd;
    App         *app;
    const char  *args[MAX_ARGS];
    u32         n_args;
} Context;

struct Command {
    const char *name;
    const char *usage;
    const char *description;
    Command    *subcommands[DISPATCH_MAX_COMMANDS];
    Flag        flags[DISPATCH_MAX_FLAGS];
    i32        (*action)(Context *ctx);
};
 

struct App {
    const char  *name;
    const char  *version;
    const char  *description;
    Command     *commands[DISPATCH_MAX_COMMANDS];
};

typedef enum {
    DISPATCH_OK,
    DISPATCH_ERR
} DispatchResult;


bool        flag_bool  (Context *ctx, const char *name);
const char *flag_str   (Context *ctx, const char *name);
i32         flag_int   (Context *ctx, const char *name);
f32         flag_float (Context *ctx, const char *name);

DispatchResult dispatch(App *app, i32 argc, char **argv);

#endif /* DISPATCH_H */