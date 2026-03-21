#include "cmd.h"
#include "ansi.h"
#include "fn.h"
#include "log.h"
#include <stdlib.h>
#include <stdio.h>


i32 cmd_new(Context *ctx) {
    if (ctx->n_args < 1) {
        err("'new' requires a project name");
        return -1;
    }
    status_info("Creating", "new project: %s", ctx->args[0]);
    i32 result = fn_new_project(ctx->args[0]);
    if (result == 0) {
        ok("Project '%s' created successfully!", ctx->args[0]);
    }
    return result;
}

i32 cmd_init(Context *ctx) {
    const char *name = ctx->n_args > 0 ? ctx->args[0] : NULL;

    status_info("Initializing", "current directory as a project");
    i32 result = fn_init(name);
    if (result == 0) {
        ok("Project initialized successfully!");
    }
    return result;
}

i32 cmd_build(Context *ctx) {
    const char *profile = flag_str(ctx, "profile");
    const char *target  = flag_str(ctx, "target");
    i32         jobs    = flag_int(ctx, "jobs");

    if (flag_bool(ctx, "release")) {
        profile = "release";
    }
    info("Project File Path: %s", flag_str(ctx, "file"));
    status("build", "[%s] target '%s' with %d jobs", profile, target, jobs);
    return 0;
}

i32 cmd_run(Context *ctx) {
    const char *profile = flag_str(ctx, "profile");
    const char *target  = flag_str(ctx, "target");
    i32         jobs    = flag_int(ctx, "jobs");

    if (flag_bool(ctx, "release")) {
        profile = "release";
    }
    info("Project File Path: %s", flag_str(ctx, "file"));
    status("build", "[%s] target '%s' with %d jobs", profile, target, jobs);
    status("run", "Running '%s'", target);
    return 0;
}

i32 cmd_clean(Context *ctx) {
    (void)ctx;
    status("clean", "Removing build artifacts");
    return 0;
}
