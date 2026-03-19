#ifndef FN_H
#define FN_H

#include "types.h"
#include "fs.h"

extern const char* PROJECT_FILE_NAME;

i32 fn_new_project(const char* project_name);

i32 fn_init(const char *project_name);


#endif /* FN_H */