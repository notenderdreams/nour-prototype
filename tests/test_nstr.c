#include "test.h"
#include "nstr.h"
#include "arena.h"

static void test_nstr_from(void) {
    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "hello");
    ASSERT_EQ_INT(s.len, 5);
    ASSERT_EQ_STR(s.data, "hello");
    arena_destroy(a);
}

static void test_nstr_from_empty(void) {
    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "");
    ASSERT_EQ_INT(s.len, 0);
    ASSERT(s.data != NULL);
    ASSERT_EQ_STR(s.data, "");
    arena_destroy(a);
}

static void test_nstr_from_null(void) {
    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, NULL);
    ASSERT_EQ_INT(s.len, 0);
    ASSERT(s.data == NULL);
    nstr s2 = nstr_from(NULL, "hello");
    ASSERT(s2.data == NULL);
    arena_destroy(a);
}

static void test_nstr_concat(void) {
    Arena *a = arena_create(1024);
    nstr x = nstr_from(a, "hello");
    nstr y = nstr_from(a, " world");
    nstr z = nstr_concat(a, x, y);
    ASSERT_EQ_INT(z.len, 11);
    ASSERT_EQ_STR(z.data, "hello world");
    arena_destroy(a);
}

static void test_nstr_concat_empty(void) {
    Arena *a = arena_create(1024);
    nstr x = nstr_from(a, "hello");
    nstr y = nstr_from(a, "");
    nstr z = nstr_concat(a, x, y);
    ASSERT_EQ_INT(z.len, 5);
    ASSERT_EQ_STR(z.data, "hello");
    arena_destroy(a);
}

static void test_nstr_append(void) {
    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "hello");
    nstr s2 = nstr_append(a, s, " world");
    ASSERT_EQ_INT(s2.len, 11);
    ASSERT_EQ_STR(s2.data, "hello world");
    arena_destroy(a);
}

static void test_nstr_append_null(void) {
    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "hello");
    nstr s2 = nstr_append(a, s, NULL);
    ASSERT(s2.data == NULL);
    arena_destroy(a);
}

static void test_nstr_cmp(void) {
    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "abc");
    ASSERT_EQ_INT(nstr_cmp(s, "abc"), 0);
    ASSERT(nstr_cmp(s, "xyz") != 0);
    ASSERT(nstr_cmp(s, "ab") != 0);
    ASSERT(nstr_cmp(s, "abcd") != 0);
    arena_destroy(a);
}

static void test_nstr_cstr(void) {
    nstr null_str = {0, NULL};
    ASSERT_EQ_STR(nstr_cstr(null_str), "");

    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "test");
    ASSERT_EQ_STR(nstr_cstr(s), "test");
    arena_destroy(a);
}

static void test_nstr_empty(void) {
    nstr null_str = {0, NULL};
    ASSERT(nstr_empty(null_str));

    Arena *a = arena_create(1024);
    nstr s = nstr_from(a, "x");
    ASSERT(!nstr_empty(s));
    nstr e = nstr_from(a, "");
    ASSERT(nstr_empty(e));
    arena_destroy(a);
}

void suite_nstr(void) {
    test_nstr_from();
    test_nstr_from_empty();
    test_nstr_from_null();
    test_nstr_concat();
    test_nstr_concat_empty();
    test_nstr_append();
    test_nstr_append_null();
    test_nstr_cmp();
    test_nstr_cstr();
    test_nstr_empty();
}
