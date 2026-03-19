#include "fn.h"
#include "fs.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>

const char* PROJECT_FILE_NAME = "project.nour";

static i32 write_project_file(const char* name, const char* path) {
    char content[1024];

    snprintf(content, sizeof(content),
        "#include \"nour.h\"\n"
        "\n"
        "Project %s = {\n"
        "    .version = \"0.1.0\",\n"
        "    .cc = \"gcc\",\n"
        "    .sources = {\n"
        "        \"src/*.c\",\n"
        "    },\n"
        "};\n",
        name
    );
    return create_file(content, path);

}
/*
    TODO:  x
    * Project Name validation (no spaces, special chars, etc.) 
    * create src/main.c
     
*/
i32 fn_new_project(const char* project_name) {
    if (!check_dir_exists(project_name)) {
        fprintf(stderr, "error: directory '%s' already exists\n", project_name);
        return -1;
    }
    if (create_dir(project_name)) {
        fprintf(stderr, "error: failed to create directory '%s'\n", project_name);
        return -2;
    }
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", project_name, PROJECT_FILE_NAME);

    return write_project_file(project_name, path);
}

i32 fn_init(const char *project_name) {
    char cwd[PATH_MAX];
    if (!project_name) {
        if (!getcwd(cwd, sizeof(cwd))) 
            return -1;
        project_name = basename(cwd);
    } 
    if (check_dir_exists(PROJECT_FILE_NAME) == 1) {
        fprintf(stderr, "error: project.nour already exists\n");
        return -1;
    }
    return write_project_file(project_name, PROJECT_FILE_NAME); 
}
