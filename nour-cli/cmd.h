#ifndef CMD_H
#define CMD_H

#include "types.h"
#include "dispatch.h"

//  Variables ---------
#define BUILD_FLAGS \
    { "profile", 'p',       FLAG_STR,  .val.s = "debug",   "build profile (debug, release)" }, \
    { "release", NO_SHORT,  FLAG_BOOL, .val.b = false,     "build in release mode"          }, \
    { "target",  't',       FLAG_STR,  .val.s = "default", "build target"                   }, \
    { "jobs",    'j',       FLAG_INT,  .val.i = 4,         "parallel jobs"                  }, \
    FLAG_END


//  Command actions --------
i32 cmd_new(Context *ctx);

i32 cmd_init(Context *ctx);

i32 cmd_build(Context *ctx);

i32 cmd_run(Context *ctx);

i32 cmd_clean(Context *ctx);

//  Command definitions --------
static Command new_cmd = {
    .name   = "new",
    .alias  = "n",
    .usage  = "Create a new project",
    .action = cmd_new,
};

static Command init_cmd = {
    .name   = "init",
    .alias  = "i",
    .usage  = "Initialize the current directory as a project",
    .action = cmd_init,
};

static Command build_cmd = {
    .name   = "build",
    .alias  = "b",
    .usage  = "Compile the project",
    .action = cmd_build,
    .flags       = { BUILD_FLAGS }
};

static Command run_cmd = {
    .name        = "run",
    .alias       = "r",
    .usage       = "Build and run the binary",
    .description = "Builds then runs the binary.\nPass -- to forward args: nour run --release -- --flag value",
    .action      = cmd_run,
    .flags       = { BUILD_FLAGS }
};

static Command clean_cmd = {
    .name   = "clean",
    .usage  = "Remove build artifacts",
    .action = cmd_clean,
};


#endif /* CMD_H */