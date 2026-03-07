#include "test.h"

int tests_run    = 0;
int tests_passed = 0;
int tests_failed = 0;

int main(void) {
    log_print(LOG_INFO, "Running nour test suite\n\n");

    RUN_SUITE(suite_arena);
    RUN_SUITE(suite_nstr);
    RUN_SUITE(suite_fs);
    RUN_SUITE(suite_log);
    RUN_SUITE(suite_incremental);

    printf("\n");
    log_print(LOG_INFO, "Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %s%d failed%s", COLOR_RED, tests_failed, COLOR_RESET);
    }
    printf("\n");

    return tests_failed > 0 ? 1 : 0;
}

