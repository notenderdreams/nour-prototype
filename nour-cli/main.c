#include <stdio.h>
#include "dispatch.h"

i32 cmd_build(Context *ctx) {
    const char *profile = flag_str(ctx, "profile");
    i32         jobs    = flag_int(ctx, "jobs");
    printf("   Compiling [%s] jobs=%d\n", profile, jobs);
    for (i32 i = 0; i < (i32)ctx->n_args; i++)
        printf("   package : %s\n", ctx->args[i]);
    return 0;
}

i32 cmd_run(Context *ctx) {
    bool        release = flag_bool(ctx, "release");
    const char *profile = release ? "release" : "debug";
    const char *binary  = "./out/cli";

    printf("   Compiling [%s]\n", profile);
    printf("   Running   %s", binary);
    for (i32 i = 0; i < (i32)ctx->n_args; i++)
        printf(" %s", ctx->args[i]);
    printf("\n");
    return 0;
}

i32 cmd_clean(Context *ctx) {
    bool all = flag_bool(ctx, "all");
    printf("   Cleaning  %s\n", all ? "everything" : "artifacts");
    return 0;
}

static Command build_cmd = {
    .name   = "build",
    .usage  = "Compile the project",
    .action = cmd_build,
    .flags  = {
        { "profile", NO_SHORT, FLAG_STR, .val.s = "debug", "build profile (debug, release)" },
        { "jobs",    'j', FLAG_INT, .val.i = 4,        "parallel jobs"                 },
        FLAG_END
    },
};

static Command run_cmd = {
    .name        = "run",
    .usage       = "Build and run the binary",
    .description = "Builds then runs the binary.\nPass -- to forward args: nour run --release -- --flag value",
    .action      = cmd_run,
    .flags = {
        { "release", 'r', FLAG_BOOL, .val.b = false, "build and run in release mode" },
        FLAG_END
    },
};

static Command clean_cmd = {
    .name   = "clean",
    .usage  = "Remove build artifacts",
    .action = cmd_clean,
    .flags  = {
        { "all", 'a', FLAG_BOOL, .val.b = false, "also clear caches" },
        FLAG_END
    },
};

int main(int argc, char **argv) {
    App app = {
        .name        = "nour",
        .version     = "0.1.0",
        .description = "A C build system.",
        .commands    = { &build_cmd, &run_cmd, &clean_cmd, CMD_END },
    };
    return dispatch(&app, argc, argv);
}