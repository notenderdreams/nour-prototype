/*
 * vector.c
 * --------
 * Uses a directory of heap-allocated blocks. The first block stores
 * VECTOR_INITIAL_CAP elements, and each subsequent block doubles in size.
 *
 * This avoids reallocating or moving previously stored elements while still
 * providing O(1) indexed lookup, push, and pop.
 */


#ifndef VECTOR_H
#define VECTOR_H

#include "types.h"
#include <stdlib.h>
#include <stdio.h>

#define VECTOR_MAX_CAPACITY   UINT32_MAX
#define VECTOR_INITIAL_CAP    8
#define VECTOR_INITIAL_LOG2   3

typedef struct {
    void **datablock;
    u32    size;
    u32    block_capacity;
    u32    block_count;
} Vector;

Vector  vector_create       (u32 max_capacity);
void   *vector_get          (Vector *vec, u32 index);
void    vector_push         (Vector *vec, void *element);
void   *vector_pop          (Vector *vec);
void    vector_destroy      (Vector *vec);

#define vec_create()         vector_create(VECTOR_MAX_CAPACITY)
#define vec_get(T, v, i)     ((T *)vector_get(&(v), (u32)(i)))
#define vec_push(v, val)     vector_push(&(v), (val))
#define vec_pop(T, v)        ((T *)vector_pop(&(v)))
#define vec_destroy(v)       vector_destroy(&(v))

#define vec_foreach(T, v, it)                          \
    for (u32 _i = 0; _i < (v).size; _i++)             \
        for (T *it = vec_get(T, v, _i); it; it = NULL)

#endif /* VECTOR_H */