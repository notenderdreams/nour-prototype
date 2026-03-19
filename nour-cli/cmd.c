#include "cmd.h"
#include "ansi.h"
#include "fn.h"
#include <stdio.h>


i32 cmd_new(Context *ctx) {
    if (ctx->n_args < 1) {
        fprintf(stderr,"%s 'new' requires a project name\n", fg.red("error:"));
        return -1;
    }    

    printf("Creating new project: %s\n", ctx->args[0]);
    return fn_new_project(ctx->args[0]);
}

i32 cmd_init(Context *ctx) {
    const char *name = ctx->n_args > 0 ? ctx->args[0] : NULL;

    printf("Initialize the current directory as a project\n");
    return fn_init(name);
}

i32 cmd_build(Context *ctx) {
    const char *profile = flag_str(ctx, "profile");
    const char *target  = flag_str(ctx, "target");
    i32         jobs    = flag_int(ctx, "jobs");

    if (flag_bool(ctx, "release")) {
        profile = "release";
    }
    printf("Project File Path: %s\n", flag_str(ctx, "file"));
    printf("   Building [%s] target '%s' with %d jobs\n", profile, target, jobs);
    return 0;
}

i32 cmd_run(Context *ctx) {
    const char *profile = flag_str(ctx, "profile");
    const char *target  = flag_str(ctx, "target");
    i32         jobs    = flag_int(ctx, "jobs");

    if (flag_bool(ctx, "release")) {
        profile = "release";
    }
    printf("Project File Path: %s\n", flag_str(ctx, "file"));
    printf("   Building [%s] target '%s' with %d jobs\n", profile, target, jobs);
    printf("   Running '%s'\n", target);
    return 0;
}

i32 cmd_clean(Context *ctx) {
    (void)ctx;
    printf("   Removing build artifacts\n");
    return 0;
}
