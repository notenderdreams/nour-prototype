#include "compile.h"
#include "nour.h"
#include "utils.h"
#include "loader.h"
#include "parser.h"
#include "log.h"
#include "fs.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

static const char *NOUR_FILE   = "sandbox/project.nour";
static const char *NOUR_C_FILE = "build/project.nour.c";
static const char *LIB_PATH    = "build/libnour.so";

static int compile_nour_so(const char *c_file, const char *so_file) {
    char *argv[] = {
        "gcc", "-shared", "-fPIC", "-include", "stddef.h", "-Isrc",
        "-o", (char *)so_file, (char *)c_file, NULL
    };

    pid_t pid = fork();
    if (pid == -1) {
        log_print(LOG_ERROR, "fork() failed\n");
        return 1;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        perror("execvp");
        _exit(1);
    }

    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        log_print(LOG_ERROR, "Failed to compile %s\n", c_file);
        return 1;
    }
    return 0;
}

int main(void) {
    if (ensure_directory("build") != 0) return 1;

    // Step 1: preprocess .nour → valid C, extract declarations
    NourDecl decls[16];
    size_t   decl_count = 0;
    if (nour_preprocess(NOUR_FILE, NOUR_C_FILE, decls, &decl_count) != 0)
        return 1;

    // Step 2: find declarations we care about
    const char *symbol = NULL;
    const char *profile_symbols[16];
    size_t profile_count = 0;
    for (size_t i = 0; i < decl_count; i++) {
        if (strcmp(decls[i].type, "Project") == 0) {
            symbol = decls[i].name;
        } else if (strcmp(decls[i].type, "Profile") == 0) {
            if (profile_count < 16) {
                profile_symbols[profile_count++] = decls[i].name;
            }
        }
    }

    // Profiles are parsed and recorded for future selection support.
    if (profile_count > 0) {
        log_print(LOG_INFO, "Found %zu profile declaration(s).", profile_count);
        for (size_t i = 0; i < profile_count; i++) {
            log_print(LOG_ALIGNED, "%s", profile_symbols[i]);
        }
    }

    if (!symbol) {
        log_print(LOG_ERROR, "No Project declaration found in %s\n", NOUR_FILE);
        return 1;
    }

    // Step 3: compile .nour.c → libnour.so
    if (compile_nour_so(NOUR_C_FILE, LIB_PATH) != 0)
        return 1;

    // Step 4: load the symbol by name
    LoadedProject lp = load_project(LIB_PATH, symbol);
    if (!lp.project)
        return 1;

    print_project(lp.project, lp.name);

    int rc = compile_project(lp.project, lp.name);
    unload_project(&lp);

    return rc != 0 ? 1 : 0;
}