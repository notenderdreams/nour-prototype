#include "nstr.h"
#include "arena.h"
#include <stddef.h>
#include <string.h>

nstr nstr_from(Arena* arena, const char *cstr){
    if (arena == NULL || cstr == NULL) {
        return (nstr){0, NULL};
    }

    size_t len = strlen(cstr);
    if (len + 1 < len) {
        return (nstr){0, NULL};
    }

    char* mem = arena_alloc(arena, len +1);

    if(!mem)
        return (nstr){0,NULL};

    memcpy(mem, cstr, len + 1);
    return (nstr){len,mem};
}

nstr nstr_concat(Arena* arena, nstr a, nstr b){
    if (arena == NULL || (a.len > 0 && a.data == NULL) || (b.len > 0 && b.data == NULL)) {
        return (nstr){0, NULL};
    }
    if (a.len > ((size_t)-1) - b.len) {
        return (nstr){0, NULL};
    }

    size_t len = a.len + b.len;
    if (len + 1 < len) {
        return (nstr){0, NULL};
    }

    char* mem = arena_alloc(arena, len+1);

    if(!mem) return (nstr){0,NULL};

    memcpy(mem, a.data, a.len);
    memcpy(mem + a.len , b.data, b.len);
    mem[len] = '\0';

    return (nstr){len, mem};
}


nstr nstr_append(Arena* arena, nstr original, const char* suffix){
    if (arena == NULL || suffix == NULL || (original.len > 0 && original.data == NULL)) {
        return (nstr){0, NULL};
    }

    size_t suffix_len = strlen(suffix);
    if (original.len > ((size_t)-1) - suffix_len) {
        return (nstr){0, NULL};
    }

    size_t total_len = original.len + suffix_len;
    if (total_len + 1 < total_len) {
        return (nstr){0, NULL};
    }

    char* mem = arena_alloc(arena, total_len + 1);
    if(!mem) return (nstr){0,NULL};

    memcpy(mem, original.data, original.len);
    memcpy(mem + original.len, suffix, suffix_len);
    mem[total_len] = '\0';

    return (nstr){total_len, mem};
}


