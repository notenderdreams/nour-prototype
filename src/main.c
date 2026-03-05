#include "compile.h"
#include "nour.h"
#include "utils.h"
#include "loader.h"

const char* FILE_PATH = "./build/libnour.so";

int main() {
    Project *project_ptr = load_project(FILE_PATH);
    if (!project_ptr) {
        return 1;
    }

    print_project(project_ptr);

    if (compile_project(project_ptr) != 0) {
        return 1;
    }

    return 0;
}
