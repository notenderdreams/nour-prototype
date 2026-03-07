#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>
#include "log.h"

// ─── Shared test counters ──────────────────────────────────────────

extern int tests_run;
extern int tests_passed;
extern int tests_failed;

// ─── Assertion macros ──────────────────────────────────────────────

#define ASSERT(expr)                                                          \
    do {                                                                      \
        tests_run++;                                                          \
        if (expr) {                                                           \
            tests_passed++;                                                   \
        } else {                                                              \
            tests_failed++;                                                   \
            log_print(LOG_ERROR, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        }                                                                     \
    } while (0)

#define ASSERT_EQ_INT(a, b)                                                   \
    do {                                                                      \
        tests_run++;                                                          \
        if ((a) == (b)) {                                                     \
            tests_passed++;                                                   \
        } else {                                                              \
            tests_failed++;                                                   \
            log_print(LOG_ERROR, "FAIL %s:%d: %d != %d\n",                   \
                      __FILE__, __LINE__, (int)(a), (int)(b));                \
        }                                                                     \
    } while (0)

#define ASSERT_EQ_STR(a, b)                                                   \
    do {                                                                      \
        tests_run++;                                                          \
        if ((a) && (b) && strcmp((a), (b)) == 0) {                            \
            tests_passed++;                                                   \
        } else {                                                              \
            tests_failed++;                                                   \
            log_print(LOG_ERROR, "FAIL %s:%d: \"%s\" != \"%s\"\n",           \
                      __FILE__, __LINE__, (a) ? (a) : "(null)",               \
                      (b) ? (b) : "(null)");                                  \
        }                                                                     \
    } while (0)

#define RUN_SUITE(fn)                                                         \
    do {                                                                      \
        log_print(LOG_INFO, "── %s\n", #fn);                                 \
        fn();                                                                 \
    } while (0)

// ─── Suite declarations ────────────────────────────────────────────

void suite_arena(void);
void suite_nstr(void);
void suite_fs(void);
void suite_log(void);
void suite_incremental(void);

#endif // TEST_H
