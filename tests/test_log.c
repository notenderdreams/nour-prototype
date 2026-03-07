#include "test.h"
#include "log.h"

static void test_log_levels(void) {
    log_print(LOG_OK,    "test ok: %d\n", 1);
    log_print(LOG_ERROR, "test error: %d\n", 2);
    log_print(LOG_WARN,  "test warn: %d\n", 3);
    log_print(LOG_INFO,  "test info: %d\n", 4);
    tests_run++;
    tests_passed++;
}

void suite_log(void) {
    test_log_levels();
}
