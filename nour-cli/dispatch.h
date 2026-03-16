#ifndef DISPATCH_H
#define DISPATCH_H

#include "types.h"
#include <stdbool.h>
#include <stddef.h>

#define DISPATCH_MAX_COMMANDS 32

typedef struct Command Command;

struct Command {
    const char  *name;
    const char  *usage;
    const char  *description;
    Command     *subcommands[DISPATCH_MAX_COMMANDS];
    i32         (*action)(const char **args, i32 n);
};

typedef struct {
    const char  *name;
    const char  *version;
    const char  *description;
    Command     *commands[DISPATCH_MAX_COMMANDS];
} App;

typedef enum {
    DISPATCH_OK,
    DISPATCH_ERR
} DispatchResult;

DispatchResult dispatch(App *app, i32 argc, char **argv);

#endif /* DISPATCH_H */