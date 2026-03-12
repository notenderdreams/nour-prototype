#ifndef COMPILE_H
#define COMPILE_H

#include "nour.h"
#include <stddef.h>

int    compile_project(const Project *project, const char *name);
size_t compile_built_count(void);
size_t compile_uptodate_count(void);
void   compile_reset_counters(void);

#endif // COMPILE_H
