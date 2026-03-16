#include <stdio.h>
#include "dispatch.h"

int cmd_build(const char **args, int n) {
    printf("building\n");
    for (int i = 0; i < n; i++) printf("  %s\n", args[i]);
    return 0;
}

int cmd_run(const char **args, int n) {
    printf("running\n");
    if (n > 0) printf("  bin: %s\n", args[0]);
    return 0;
}

int cmd_clean(const char **args, int n) {
    (void)args; (void)n;
    printf("cleaning\n");
    return 0;
}

int cmd_log_show(const char **args, int n) {
    (void)args; (void)n;
    printf("log show\n");
    return 0;
}

int cmd_log_clear(const char **args, int n) {
    (void)args; (void)n;
    printf("log clear\n");
    return 0;
}

static Command log_show  = { 
    .name="show",  
    .usage="Print build log", 
    .action=cmd_log_show  
};
static Command log_clear = { 
    .name="clear", 
    .usage="Clear build log", 
    .action=cmd_log_clear 
};

static Command log_cmd   = { 
    .name="log",   
    .usage="Build log management",
    .subcommands={ &log_show, &log_clear, NULL } 
};
static Command build_cmd = { 
    .name="build", 
    .usage="Compile the project",
    .action=cmd_build 
};
static Command run_cmd   = { 
    .name="run",   
    .usage="Run a binary",        
    .action=cmd_run   
};
static Command clean_cmd = { 
    .name="clean", 
    .usage="Remove artifacts",    
    .action=cmd_clean 
};

int main(int argc, char **argv) {
    App app = {
        .name        = "nour",
        .version     = "0.1.0",
        .description = "A C build system.",
        .commands    = { &build_cmd, &run_cmd, &clean_cmd, &log_cmd, NULL },
    };
    return dispatch(&app, argc, argv);
}