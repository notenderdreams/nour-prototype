#include "compile.h"
#include "nour.h"
#include "utils.h"
#include "loader.h"

static const char* LIB_PATH = "./build/libnour.so";

int main(void) {
    LoadedProject lp = load_project(LIB_PATH);
    if (!lp.project) {
        return 1;
    }

    print_project(lp.project);

    int rc = compile_project(lp.project);
    unload_project(&lp);

    return rc != 0 ? 1 : 0;
}