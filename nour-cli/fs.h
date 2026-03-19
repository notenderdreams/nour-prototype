#ifndef FS_H
#define FS_H
/*
    Well we do have a fs in the nour-core part , but rightnow 
    I just don't want to connect the cli to the core, so this
    is a temporary fs for the cli part.
*/

#include "types.h"

i32 check_dir_exists(const char *path);

i32 create_dir(const char *path);

i32 remove_dir(const char *path);

i32 create_file(const char *file_path, const char *content);

#endif /* FS_H */