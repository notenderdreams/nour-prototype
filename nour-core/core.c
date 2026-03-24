#include "core.h"
#include "preprocess.h"

#include <stdio.h>
#include <sys/stat.h>

const char* nour_build(const char *file_path, const char *target,
                       const char *profile, u8 jobs)
{
    (void)target;
    (void)profile;
    (void)jobs;

    mkdir("build", 0755);

    static const char *preprocessed_path = "build/project.nour.c";
    Vector decls = vec_create();

    if (nour_preprocess(file_path, preprocessed_path, &decls) != 0)
        return NULL;

    return preprocessed_path;
}