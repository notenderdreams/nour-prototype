#include "test.h"
#include "fs.h"
#include "arena.h"

#include <stdio.h>
#include <unistd.h>

static void test_ensure_directory(void) {
    int rc = ensure_directory("/tmp/nour_test_dir");
    ASSERT_EQ_INT(rc, 0);
    rc = ensure_directory("/tmp/nour_test_dir");
    ASSERT_EQ_INT(rc, 0);
    rmdir("/tmp/nour_test_dir");
}

static void test_get_mtime(void) {
    time_t t = get_mtime("/tmp/nour_nonexistent_file_xyz");
    ASSERT_EQ_INT(t, 0);

    FILE *f = fopen("/tmp/nour_test_mtime", "w");
    if (f) {
        fprintf(f, "test");
        fclose(f);
        time_t mt = get_mtime("/tmp/nour_test_mtime");
        ASSERT(mt > 0);
        unlink("/tmp/nour_test_mtime");
    }
}

static void test_expand_glob(void) {
    Arena *a = arena_create(4096);
    FileList fl = expand_glob(a, "sandbox/*.c");
    ASSERT(fl.count >= 2);
    ASSERT(fl.files != NULL);
    arena_destroy(a);
}

static void test_expand_glob_no_match(void) {
    Arena *a = arena_create(4096);
    FileList fl = expand_glob(a, "/tmp/nour_no_match_*.zzz");
    ASSERT_EQ_INT(fl.count, 0);
    arena_destroy(a);
}

static void test_expand_glob_null(void) {
    Arena *a = arena_create(4096);
    FileList fl = expand_glob(a, NULL);
    ASSERT_EQ_INT(fl.count, 0);
    FileList fl2 = expand_glob(NULL, "*.c");
    ASSERT_EQ_INT(fl2.count, 0);
    arena_destroy(a);
}

static void test_get_dependent_files(void) {
    Arena *a = arena_create(4096);
    FileList deps = get_dependent_files(a, "sandbox/basic.c");
    ASSERT(deps.count >= 2);
    int found_another = 0;
    int found_boxbox = 0;
    for (size_t i = 0; i < deps.count; i++) {
        if (strstr(deps.files[i], "another.h")) found_another = 1;
        if (strstr(deps.files[i], "boxbox.h"))  found_boxbox = 1;
    }
    ASSERT(found_another);
    ASSERT(found_boxbox);
    arena_destroy(a);
}

static void test_get_dependent_files_no_deps(void) {
    Arena *a = arena_create(4096);
    FILE *f = fopen("/tmp/nour_test_nodeps.c", "w");
    if (f) {
        fprintf(f, "#include <stdio.h>\nint main() { return 0; }\n");
        fclose(f);
        FileList deps = get_dependent_files(a, "/tmp/nour_test_nodeps.c");
        ASSERT_EQ_INT(deps.count, 0);
        unlink("/tmp/nour_test_nodeps.c");
    }
    arena_destroy(a);
}

static void test_get_dependent_files_nonexistent(void) {
    Arena *a = arena_create(4096);
    FileList deps = get_dependent_files(a, "/tmp/nour_no_such_file.c");
    ASSERT_EQ_INT(deps.count, 0);
    arena_destroy(a);
}

static void test_build_dep_graph(void) {
    Arena *a = arena_create(8192);
    FileList fl = expand_glob(a, "sandbox/*.c");
    ASSERT(fl.count >= 2);

    DepGraph graph = build_dep_graph(a, fl);
    ASSERT(graph.count > 0);
    ASSERT(graph.nodes != NULL);

    int found = 0;
    for (size_t i = 0; i < graph.count; i++) {
        if (strstr(graph.nodes[i].file, "another.h")) {
            found = 1;
            ASSERT(graph.nodes[i].count >= 2);
        }
    }
    ASSERT(found);

    arena_destroy(a);
}

static void test_build_dep_graph_empty(void) {
    Arena *a = arena_create(4096);
    FileList empty = {NULL, 0};
    DepGraph graph = build_dep_graph(a, empty);
    ASSERT_EQ_INT(graph.count, 0);
    arena_destroy(a);
}

void suite_fs(void) {
    test_ensure_directory();
    test_get_mtime();
    test_expand_glob();
    test_expand_glob_no_match();
    test_expand_glob_null();
    test_get_dependent_files();
    test_get_dependent_files_no_deps();
    test_get_dependent_files_nonexistent();
    test_build_dep_graph();
    test_build_dep_graph_empty();
}
