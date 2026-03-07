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