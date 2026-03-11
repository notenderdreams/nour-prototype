#include <stdio.h>
#include "another.h"
#include "boxbox.h"
#include "math/math_ops.h"
#include "utils/strutil.h"
#include "example_cstatic.h"
#include "example_cstatic_extra.h"
#include "example_cdynamic.h"
#include "example_cdynamic_extra.h"

int main() {
    greet("nour");
    printf("another_function: %d\n", another_function());
    printf("add(3, 4): %d\n", add(3, 4));
    printf("multiply(3, 4): %d\n", multiply(3, 4));
    printf("static_square(5): %d\n", static_square(5));
    printf("static_cube(3): %d\n", static_cube(3));
    printf("static_inc_by_ten(8): %d\n", static_inc_by_ten(8));
    printf("static_dec_by_three(8): %d\n", static_dec_by_three(8));
    printf("dynamic_double(7): %d\n", dynamic_double(7));
    printf("dynamic_negate(42): %d\n", dynamic_negate(42));
    printf("dynamic_triple(7): %d\n", dynamic_triple(7));
    printf("dynamic_abs(-21): %d\n", dynamic_abs(-21));
    return 0;
}