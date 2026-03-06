#ifndef NSTR_H
#define NSTR_H
#include "arena.h"
#include <stddef.h>
#include <string.h>

typedef struct{
    size_t len;
    char* data;
} nstr;

nstr nstr_from(Arena* arena, const char *cstr);

nstr nstr_concat(Arena* arena, nstr a, nstr b);

nstr nstr_append(Arena* arena, nstr original, const char* suffix);

static inline int nstr_cmp(nstr s, const char *cstr) {
    if (!s.data || !cstr) return s.data ? 1 : (cstr ? -1 : 0);
    size_t cstr_len = strlen(cstr);
    if (s.len != cstr_len) return s.len < cstr_len ? -1 : 1;
    return memcmp(s.data, cstr, s.len);
}

static inline const char* nstr_cstr(nstr s) {
    return s.data ? s.data : "";
}

static inline int nstr_empty(nstr s) {
    return s.len == 0 || !s.data;
}

#endif // NSTR_H
