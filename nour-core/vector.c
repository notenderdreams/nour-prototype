#include "vector.h"

/*
 * Returns the datablock index that contains element slot `n`.
 *
 * Slot layout:
 *   block 0 -> [0 .. VECTOR_INITIAL_CAP - 1]
 *   block 1 -> next doubled range
 *   ...
 * Used by push/get/pop 
 */
static inline u32 block_index(u32 n) {
    if (n < VECTOR_INITIAL_CAP) return 0;
    return 31 - __builtin_clz(n >> VECTOR_INITIAL_LOG2) + 1;
}


/*
 * Returns the starting logical element index of `block`.
 *
 * Example:
 *   block 0 -> 0
 *   block 1 -> VECTOR_INITIAL_CAP
 *   block 2 -> VECTOR_INITIAL_CAP + previous block size
 *
 * This is used to convert a global vector index into an offset
 * local to a specific datablock.
 */
static inline u32 block_start(u32 block) {
    if (block == 0) return 0;
    return VECTOR_INITIAL_CAP * (1u << (block - 1));
}

/*
 * Returns the element capacity of a given datablock.
 *
 * Block 0 has VECTOR_INITIAL_CAP entries.
 * Every following block doubles relative to the previous one.
 */
static inline u32 block_size(u32 block) {
    if (block == 0) return VECTOR_INITIAL_CAP;
    return VECTOR_INITIAL_CAP * (1u << (block - 1));
}

/*
 * Initializes a new segmented vector with room for a datablock directory
 * large enough to support up to `max_capacity` elements.
 *
 * The first datablock is allocated immediately so the vector is ready
 * for push operations without extra setup.
 *
 * Returns the initialized Vector by value.
 */
Vector vector_create(u32 max_capacity) {
    Vector vec;
    vec.size           = 0;
    vec.block_capacity = block_index(max_capacity) + 1;
    vec.block_count    = 1;
    vec.datablock      = malloc(vec.block_capacity * sizeof(void *));
    if (!vec.datablock) { perror("malloc"); exit(EXIT_FAILURE); }
    vec.datablock[0]   = malloc(VECTOR_INITIAL_CAP * sizeof(void *));
    if (!vec.datablock[0]) { perror("malloc"); exit(EXIT_FAILURE); }
    return vec;
}
/*
 * Returns the element stored at logical index `index`.
 */
void *vector_get(Vector *vec, u32 index) {
    if (index >= vec->size) {
        fprintf(stderr, "vector_get: index %u out of bounds (size %u)\n",
                index, vec->size);
        exit(EXIT_FAILURE);
    }
    u32 block = block_index(index);
    u32 entry = index - block_start(block);
    return ((void **)vec->datablock[block])[entry];
}

/*
 * Appends `element` to the end of the vector.
 *
 * If the current tail datablock is full, a new datablock is allocated
 * and linked into the directory. Existing blocks are never moved.
 *
 * Aborts if the vector exceeds its supported capacity.
 */
void vector_push(Vector *vec, void *element) {
    if (vec->size == VECTOR_MAX_CAPACITY) {
        fprintf(stderr, "vector_push: capacity exceeded\n");
        exit(EXIT_FAILURE);
    }
    u32 block = block_index(vec->size);
    if (block >= vec->block_count) {
        u32 bsize = block_size(block);
        vec->datablock[block] = malloc(bsize * sizeof(void *));
        if (!vec->datablock[block]) { perror("malloc"); exit(EXIT_FAILURE); }
        vec->block_count++;
    }
    u32 entry = vec->size - block_start(block);
    ((void **)vec->datablock[block])[entry] = element;
    vec->size++;
}

/*
 * Removes and returns the last element in the vector.
 *
 * If removing the last element of a non-zero datablock empties that block,
 * the datablock is freed immediately to release memory.
 *
 * Aborts if the vector is empty.
 */
void *vector_pop(Vector *vec) {
    if (vec->size == 0) {
        fprintf(stderr, "vector_pop: vector is empty\n");
        exit(EXIT_FAILURE);
    }
    vec->size--;
    u32 block = block_index(vec->size);
    u32 entry = vec->size - block_start(block);
    void *el  = ((void **)vec->datablock[block])[entry];
    if (entry == 0 && block > 0) {
        free(vec->datablock[block]);
        vec->datablock[block] = NULL;
        vec->block_count--;
    }
    return el;
}

/*
 *Frees all allocated datablocks and the datablock directory itself
 */
void vector_destroy(Vector *vec) {
    for (u32 i = 0; i < vec->block_count; i++)
        free(vec->datablock[i]);
    free(vec->datablock);
    vec->datablock      = NULL;
    vec->size           = 0;
    vec->block_count    = 0;
    vec->block_capacity = 0;
}