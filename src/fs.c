#include "fs.h"

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <glob.h>

int ensure_directory(const char *path) {
    if (mkdir(path, 0755) == 0) {
        return 0;
    }

    if (errno == EEXIST) {
        return 0;
    }

    perror("mkdir failed");
    return -1;
}

FileList expand_glob(Arena *arena, const char *pattern) {
    FileList result = {NULL, 0};

    if (!arena || !pattern) return result;

    glob_t g;
    int ret = glob(pattern, GLOB_NOSORT, NULL, &g);

    if (ret != 0) {
        if (ret == GLOB_NOMATCH) {
            fprintf(stderr, "No files matched pattern: %s\n", pattern);
        } else {
            fprintf(stderr, "glob() failed for pattern: %s\n", pattern);
        }
        globfree(&g);
        return result;
    }

    char **files = arena_alloc(arena, sizeof(char *) * g.gl_pathc);
    if (!files) {
        globfree(&g);
        return result;
    }

    for (size_t i = 0; i < g.gl_pathc; i++) {
        size_t len = strlen(g.gl_pathv[i]);
        char *copy = arena_alloc(arena, len + 1);
        if (!copy) {
            globfree(&g);
            return result;
        }
        memcpy(copy, g.gl_pathv[i], len + 1);
        files[i] = copy;
    }

    result.files = files;
    result.count = g.gl_pathc;

    globfree(&g);
    return result;
}

FileList get_dependent_files(Arena *arena, const char *filepath){
    FileList result = {NULL, 0};

    if (!arena || !filepath) return result;

    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "Failed to open file: %s\n", filepath);
        return result;
    }

    // First pass: count #include "..." directives
    size_t count = 0;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "#include \"", 10) == 0) {
            count++;
        }
    }

    if (count == 0) {
        fclose(f);
        return result;
    }

    char **files = arena_alloc(arena, sizeof(char *) * count);
    if (!files) {
        fclose(f);
        return result;
    }

    // Extract the directory from filepath to resolve relative includes
    const char *last_slash = strrchr(filepath, '/');
    size_t dir_len = last_slash ? (size_t)(last_slash - filepath + 1) : 0;

    // Second pass: extract filenames
    rewind(f);
    size_t idx = 0;

    while (fgets(line, sizeof(line), f) && idx < count) {
        if (strncmp(line, "#include \"", 10) == 0) {
            char dep[256];
            // Parse the filename between quotes
            if (sscanf(line, "#include \"%255[^\"]\"", dep) == 1) {
                size_t dep_len = strlen(dep);
                size_t total_len = dir_len + dep_len;
                char *copy = arena_alloc(arena, total_len + 1);
                if (!copy) {
                    fclose(f);
                    result.files = files;
                    result.count = idx;
                    return result;
                }
                // Prepend the directory of the source file
                if (dir_len > 0) {
                    memcpy(copy, filepath, dir_len);
                }
                memcpy(copy + dir_len, dep, dep_len + 1);
                files[idx++] = copy;
            }
        }
    }

    fclose(f);

    result.files = files;
    result.count = idx;
    return result;
}