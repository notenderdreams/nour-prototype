#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct {
    char* mem;
    size_t capacity;
    size_t offset;
} Arena;

Arena* arena_create(size_t size);
void arena_destroy(Arena* arena);
void* arena_alloc(Arena* arena, size_t size);
void arena_reset(Arena* arena);

#endif // ARENA_H
