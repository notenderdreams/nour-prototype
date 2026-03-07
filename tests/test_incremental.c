#include "test.h"
#include "fs.h"
#include "arena.h"

#include <stdio.h>
#include <unistd.h>

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

static void test_incremental_fresh_build(void) {
    ensure_directory("/tmp/nour_inc_test");
    ensure_directory("/tmp/nour_inc_test/src");
    ensure_directory("/tmp/nour_inc_test/build");

    write_file("/tmp/nour_inc_test/src/main.c",
               "#include \"helper.h\"\nint main() { return helper(); }\n");
    write_file("/tmp/nour_inc_test/src/helper.c",
               "#include \"helper.h\"\nint helper() { return 0; }\n");
    write_file("/tmp/nour_inc_test/src/helper.h",
               "#ifndef HELPER_H\n#define HELPER_H\nint helper();\n#endif\n");

    Arena *a = arena_create(4096);
    FileList fl = expand_glob(a, "/tmp/nour_inc_test/src/*.c");
    ASSERT_EQ_INT(fl.count, 2);

    for (size_t i = 0; i < fl.count; i++) {
        time_t obj_mt = get_mtime("/tmp/nour_inc_test/build/does_not_exist.o");
        ASSERT_EQ_INT(obj_mt, 0);
    }

    arena_destroy(a);

    unlink("/tmp/nour_inc_test/src/main.c");
    unlink("/tmp/nour_inc_test/src/helper.c");
    unlink("/tmp/nour_inc_test/src/helper.h");
    rmdir("/tmp/nour_inc_test/src");
    rmdir("/tmp/nour_inc_test/build");
    rmdir("/tmp/nour_inc_test");
}

static void test_incremental_mtime_comparison(void) {
    write_file("/tmp/nour_inc_src.c", "int x = 1;\n");
    sleep(1);
    write_file("/tmp/nour_inc_obj.o", "fake obj\n");

    time_t src_mt = get_mtime("/tmp/nour_inc_src.c");
    time_t obj_mt = get_mtime("/tmp/nour_inc_obj.o");
    ASSERT(obj_mt >= src_mt);

    sleep(1);
    write_file("/tmp/nour_inc_src.c", "int x = 2;\n");
    src_mt = get_mtime("/tmp/nour_inc_src.c");
    ASSERT(src_mt > obj_mt);

    unlink("/tmp/nour_inc_src.c");
    unlink("/tmp/nour_inc_obj.o");
}

static void test_incremental_header_triggers_rebuild(void) {
    ensure_directory("/tmp/nour_hdr_test");
    write_file("/tmp/nour_hdr_test/a.c", "#include \"a.h\"\nint x;\n");
    write_file("/tmp/nour_hdr_test/a.h", "#define A 1\n");

    Arena *a = arena_create(4096);
    FileList deps = get_dependent_files(a, "/tmp/nour_hdr_test/a.c");
    ASSERT_EQ_INT(deps.count, 1);
    ASSERT(strstr(deps.files[0], "a.h") != NULL);

    sleep(1);
    write_file("/tmp/nour_hdr_test/a.o", "fake");
    time_t obj_mt = get_mtime("/tmp/nour_hdr_test/a.o");

    time_t hdr_mt = get_mtime("/tmp/nour_hdr_test/a.h");
    ASSERT(obj_mt >= hdr_mt);

    sleep(1);
    write_file("/tmp/nour_hdr_test/a.h", "#define A 2\n");
    hdr_mt = get_mtime("/tmp/nour_hdr_test/a.h");
    ASSERT(hdr_mt > obj_mt);

    arena_destroy(a);

    unlink("/tmp/nour_hdr_test/a.c");
    unlink("/tmp/nour_hdr_test/a.h");
    unlink("/tmp/nour_hdr_test/a.o");
    rmdir("/tmp/nour_hdr_test");
}

static void test_incremental_dep_propagation(void) {
    ensure_directory("/tmp/nour_prop_test");
    write_file("/tmp/nour_prop_test/x.c", "#include \"common.h\"\nint x;\n");
    write_file("/tmp/nour_prop_test/y.c", "#include \"common.h\"\nint y;\n");
    write_file("/tmp/nour_prop_test/common.h", "#define C 1\n");

    Arena *a = arena_create(8192);

    char *srcs[] = {"/tmp/nour_prop_test/x.c", "/tmp/nour_prop_test/y.c"};
    FileList fl = {srcs, 2};
    DepGraph graph = build_dep_graph(a, fl);

    ASSERT(graph.count >= 1);
    int found = 0;
    for (size_t i = 0; i < graph.count; i++) {
        if (strstr(graph.nodes[i].file, "common.h")) {
            found = 1;
            ASSERT_EQ_INT(graph.nodes[i].count, 2);
        }
    }
    ASSERT(found);

    arena_destroy(a);

    unlink("/tmp/nour_prop_test/x.c");
    unlink("/tmp/nour_prop_test/y.c");
    unlink("/tmp/nour_prop_test/common.h");
    rmdir("/tmp/nour_prop_test");
}

void suite_incremental(void) {
    test_incremental_fresh_build();
    test_incremental_mtime_comparison();
    test_incremental_header_triggers_rebuild();
    test_incremental_dep_propagation();
}
