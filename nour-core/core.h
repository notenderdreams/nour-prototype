#ifndef CORE_H
#define CORE_H

#include "types.h"

const char* nour_build(const char *file_path, const char *target,
                       const char *profile, u8 jobs);

#endif /* CORE_H */