#include "dispatch.h"
#include "cmd.h"
#include <stdio.h>

int main(int argc, char **argv) {
    App app = {
        .name        = "nour",
        .version     = "0.1.0",
        .description = "A C build system.",
        .commands    = { 
            &new_cmd,
            &init_cmd,
            &build_cmd, 
            &run_cmd, 
            &clean_cmd, 
            CMD_END 
        },
    };
    return dispatch(&app, argc, argv);
}