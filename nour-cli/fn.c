#include "fn.h"
#include "fs.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <libgen.h>
#include <ctype.h>

const char* PROJECT_FILE_NAME = "project.nour";

static bool is_valid_project_name(const char* name) {
    if (!isalpha(*name) && *name != '_') 
        return false;

    for (const char* p = name + 1; *p; ++p) 
        if (!isalnum(*p) && *p != '_')
            return false;

    return true;
}

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
    return create_file(path, content);

}

static i32 write_main_file(const char* name, bool is_new_project) {
    const char* basedir = is_new_project ? name : ".";
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/src", basedir);

    i32 result = create_dir(path);
    if (result) {
        fprintf(stderr, "error: failed to create 'src' directory\n");
        return result;
    }
   
    char content[1024];
    snprintf(content, sizeof(content),
        "#include <stdio.h>\n"
        "\n"
        "int main() {\n"
        "    printf(\"Hello, %s!\\n\");\n"
        "    return 0;\n"
        "}\n",
        name
    );

    snprintf(path, sizeof(path), "%s/src/main.c", basedir); 
    return create_file(path, content);
}

i32 fn_new_project(const char* project_name) {
    if (!is_valid_project_name(project_name)) {
        fprintf(stderr, "error: invalid project name '%s'\n", project_name);
        return -1;
    }

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

    i32 result = write_project_file(project_name, path);
    if (result) 
        return result;
    
    return write_main_file(project_name, true);
}

i32 fn_init(const char *project_name) {
    char name_buffer[PATH_MAX];
    
    if (!project_name) {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd))) return -1;
        project_name = basename(cwd);
    }
    
    strncpy(name_buffer, project_name, PATH_MAX);

    if (!is_valid_project_name(name_buffer)) {
        fprintf(stderr, "error: current directory name '%s' is invalid for a C identifier\n", name_buffer);
        return -1;
    }

    if (check_dir_exists(PROJECT_FILE_NAME) == 0) {
        fprintf(stderr, "error: %s already exists in this directory\n", PROJECT_FILE_NAME);
        return -1;
    }

    if (write_project_file(name_buffer, PROJECT_FILE_NAME))
        return -2;
    return write_main_file(name_buffer, false);
}
