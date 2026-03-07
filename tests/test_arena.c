#include "test.h"
#include "arena.h"

static void test_arena_create_destroy(void) {
    Arena *a = arena_create(1024);
    ASSERT(a != NULL);
    ASSERT(a->capacity >= 1024);
    ASSERT_EQ_INT(a->offset, 0);
    arena_destroy(a);
}

static void test_arena_min_size(void) {
    Arena *a = arena_create(1);
    ASSERT(a != NULL);
    ASSERT(a->capacity >= 1024);
    arena_destroy(a);
}

static void test_arena_alloc_basic(void) {
    Arena *a = arena_create(4096);
    void *p1 = arena_alloc(a, 16);
    void *p2 = arena_alloc(a, 32);
    ASSERT(p1 != NULL);
    ASSERT(p2 != NULL);
    ASSERT(p2 > p1);
    arena_destroy(a);
}

static void test_arena_alloc_zero(void) {
    Arena *a = arena_create(1024);
    void *p = arena_alloc(a, 0);
    ASSERT(p == NULL);
    arena_destroy(a);
}

static void test_arena_alloc_null(void) {
    void *p = arena_alloc(NULL, 16);
    ASSERT(p == NULL);
}

static void test_arena_reset(void) {
    Arena *a = arena_create(1024);
    arena_alloc(a, 256);
    ASSERT(a->offset > 0);
    arena_reset(a);
    ASSERT_EQ_INT(a->offset, 0);
    arena_destroy(a);
}

static void test_arena_grow(void) {
    Arena *a = arena_create(1024);
    void *p = arena_alloc(a, 2048);
    ASSERT(p != NULL);
    ASSERT(a->capacity >= 2048);
    arena_destroy(a);
}

static void test_arena_alignment(void) {
    Arena *a = arena_create(1024);
    arena_alloc(a, 1);
    ASSERT(a->offset % 8 == 0);
    arena_alloc(a, 3);
    ASSERT(a->offset % 8 == 0);
    arena_destroy(a);
}

void suite_arena(void) {
    test_arena_create_destroy();
    test_arena_min_size();
    test_arena_alloc_basic();
    test_arena_alloc_zero();
    test_arena_alloc_null();
    test_arena_reset();
    test_arena_grow();
    test_arena_alignment();
}
