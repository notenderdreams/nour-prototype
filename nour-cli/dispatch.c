#include "dispatch.h"
#include <stdio.h>
#include <string.h>

#define MAX_ARGS 64

static Command *find_cmd(Command **cmds, const char *name) {
    for (u32 i=0; cmds[i]; ++i) 
        if (strcmp(cmds[i]->name, name) == 0)
            return cmds[i];
    return NULL;
}

static void help_app(App *app) {
    printf("   %s - %s\n", app->name, app->description ? app->description : "");
    printf("\nUSAGE:\n");
    printf("   %s [global options] [command [command options]]\n", app->name);
    if (app->version) {
        printf("\nVERSION:\n");
        printf("   %s\n", app->version);
    }
    printf("\nCOMMANDS:\n");
    for (u32 i = 0; app->commands[i]; ++i)
        printf("   %-16s %s\n", app->commands[i]->name,
               app->commands[i]->usage ? app->commands[i]->usage : "");
}

static void help_cmd(App *app, Command *cmd) {
    printf("   %s %s - %s\n", app->name, cmd->name,
           cmd->description ? cmd->description : (cmd->usage ? cmd->usage : ""));
    printf("\nUSAGE:\n");
    printf("   %s %s [options]\n", app->name, cmd->name);
    if (cmd->subcommands[0]) {
        printf("\nCOMMANDS:\n");
        for (u32 i = 0; cmd->subcommands[i]; ++i)
            printf("   %-16s %s\n", cmd->subcommands[i]->name,
                   cmd->subcommands[i]->usage ? cmd->subcommands[i]->usage : "");
    }
}

static void show_version(App *app) {
    if (app->version)
        printf("%s v%s\n", app->name, app->version);
    else
        printf("%s version unknown\n", app->name);
}


DispatchResult dispatch(App *app, i32 argc, char **argv) {
    if (argc < 2) {
        help_app(app);
        return DISPATCH_OK;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        help_app(app);
        return DISPATCH_OK;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        show_version(app);
        return DISPATCH_OK;
    }

    Command  *cmd       = NULL;
    Command **search_in = app->commands;
    u32       arg_start = 1;

    while (arg_start < argc) {
        const char *tok = argv[arg_start];
        if (tok[0] == '-') break;
        Command *found = find_cmd(search_in, tok);
        if (!found) break;
        cmd       = found;
        search_in = cmd->subcommands;
        arg_start++;
    }

    if (!cmd) {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        fprintf(stderr, "Run '%s --help' for usage.\n", app->name);
        return DISPATCH_ERR;
    }

    for (u32 i = arg_start; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            help_cmd(app, cmd);
            return DISPATCH_OK;
        }
    }

    if (!cmd->action) { help_cmd(app, cmd); return DISPATCH_OK; }

    const char *args[MAX_ARGS];
    u32 n = 0;
    for (u32 i = arg_start; i < argc && n < MAX_ARGS; ++i)
        args[n++] = argv[i];

    return cmd->action(args, n) == 0 ? DISPATCH_OK : DISPATCH_ERR;
}