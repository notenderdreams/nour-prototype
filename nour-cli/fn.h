#ifndef FN_H
#define FN_H

#include "types.h"
#include "fs.h"

extern const char* PROJECT_FILE_NAME;

typedef struct {
    const char* file;
    const char* target;
    const char* profile;
    u8 jobs;
}BuildInfo ;

i32 fn_new_project(const char* project_name);

i32 fn_init(const char *project_name);

i32 fn_build(const BuildInfo *info);

#endif /* FN_H */