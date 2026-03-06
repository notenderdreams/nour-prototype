#include "arena.h"
#include <stdlib.h>

#define ARENA_MIN_SIZE 1024

Arena* arena_create(size_t capacity) {
    if (capacity < ARENA_MIN_SIZE) capacity = ARENA_MIN_SIZE;
    
    Arena* arena = malloc(sizeof(Arena));
    if(!arena) return NULL;
    
    arena->mem = malloc(capacity);
    if(!arena->mem){
        free(arena);
        return NULL;
    }

    arena->capacity = capacity;
    arena->offset = 0;

    return arena;
}

void arena_destroy(Arena* arena) {
    if(!arena) return;

    free(arena->mem);
    free(arena);
}

static int arena_grow(Arena* arena, size_t required_size) {
    if (required_size > ((size_t)-1) - arena->offset) {
        return -1;
    }

    size_t required_total = arena->offset + required_size;
    size_t new_capacity = arena->capacity;
    
    // Calculate new capacity with 1.5x growth factor to reduce reallocations
    while(required_total > new_capacity) {
        size_t next = (new_capacity * 3) / 2;  
        if (next <= new_capacity) {  
            if (required_total > ((size_t)-1) - ARENA_MIN_SIZE) {
                return -1;
            }
            next = required_total + ARENA_MIN_SIZE;
        }
        new_capacity = next;
    }
    
    char* new_memory = realloc(arena->mem, new_capacity);
    if(!new_memory) return -1;

    arena->mem = new_memory;
    arena->capacity = new_capacity;

    return 0;
}

void* arena_alloc(Arena* arena, size_t size) {
    if(!arena || size == 0) return NULL;
    if (size > ((size_t)-1) - 7) return NULL;

    // Align to 8 bytes for better memory access patterns
    size_t aligned_size = (size + 7) & ~7;
    if (aligned_size > ((size_t)-1) - arena->offset) return NULL;
    
    if(arena->offset + aligned_size > arena->capacity) {
        if(arena_grow(arena, aligned_size) != 0)
            return NULL;
    }
    
    void* ptr = arena->mem + arena->offset;
    arena->offset += aligned_size;
    return ptr;
}

void arena_reset(Arena* arena) {
    if(!arena) return;
    arena->offset = 0;
}
