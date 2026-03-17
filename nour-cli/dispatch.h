#ifndef DISPATCH_H
#define DISPATCH_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

#define DISPATCH_MAX_COMMANDS   32
#define MAX_ARGS                64


typedef struct Command  Command;
typedef struct App      App;

typedef struct {
    Command     *cmd;
    App         *app;
    const char  *args[MAX_ARGS];
    u32         n_args;
} Context;

struct Command {
    const char  *name;
    const char  *usage;
    const char  *description;
    Command     *subcommands[DISPATCH_MAX_COMMANDS];
    i32         (*action)(Context *ctx);
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

DispatchResult dispatch(App *app, i32 argc, char **argv);

#endif /* DISPATCH_H */