#include "fn.h"
#include "fs.h"
#include "log.h"
#include "core.h"

#include <stdlib.h>
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
    char content[2048];

    snprintf(content, sizeof(content),
        "#include \"nour.h\"\n"
        "\n"
        "Project %s = {\n"
        "    .version = \"0.1.0\",\n"
        "    .cc      = \"gcc\",\n"
        "};\n"
        "\n"
        "Executable %s = {\n"
        "    .sources  = {\"src/*.c\"},\n"
        "    .includes = {\"src\"},\n"
        "};\n",
        name, name
    );

    i32 result = create_file(path, content);
    if (result == 0)
        status("Created", "%s", path);
    return result;
}

static i32 write_main_file(const char* name, bool is_new_project) {
    const char* basedir = is_new_project ? name : ".";
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/src", basedir);

    i32 result = create_dir(path);
    if (result) {
        err("failed to create 'src' directory");
        return result;
    }
    status("Created", "%s/", path);
   
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
    result = create_file(path, content);
    if (result == 0) {
        status("Created", "%s", path);
    }
    return result;
}

i32 fn_new_project(const char* project_name) {
    if (!is_valid_project_name(project_name)) {
        err("invalid project name '%s'", project_name);
        return -1;
    }

    if (!check_dir_exists(project_name)) {
        err("directory '%s' already exists", project_name);
        return -1;
    }

    if (create_dir(project_name)) {
        err("failed to create directory '%s'", project_name);
        return -2;
    }
    status("Created", "%s/", project_name);
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
        err("current directory name '%s' is invalid for a C identifier", name_buffer);
        return -1;
    }

    if (check_dir_exists(PROJECT_FILE_NAME) == 0) {
        err("%s already exists in this directory", PROJECT_FILE_NAME);
        return -1;
    }

    if (write_project_file(name_buffer, PROJECT_FILE_NAME))
        return -2;
    return write_main_file(name_buffer, false);
}


i32 fn_build(const BuildInfo *binfo) {
    if (!binfo->file) {
        err("no project file specified");
        return -1;
    }

    if (check_dir_exists(binfo->file) != 1) {
        err("project file '%s' not found", binfo->file);
        return -1;
    }

    create_dir("build");

    status_info("Building", "%s [profile: %s, jobs: %d]",
        binfo->file,
        binfo->profile ? binfo->profile : "debug",
        binfo->jobs);

    const char *result = nour_build(binfo->file, binfo->target,
                                    binfo->profile, binfo->jobs);
    if (!result) {
        err("build failed");
        return -1;
    }

    status("Preprocessed", "%s", result);
    return 0;
}