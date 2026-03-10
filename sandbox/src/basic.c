#include <stdio.h>
#include "another.h"
#include "boxbox.h"
#include "math/math_ops.h"
#include "utils/strutil.h"
#include "example_cstatic.h"
#include "example_cdynamic.h"

int main() {
    greet("nour");
    printf("another_function: %d\n", another_function());
    printf("add(3, 4): %d\n", add(3, 4));
    printf("multiply(3, 4): %d\n", multiply(3, 4));
    printf("static_square(5): %d\n", static_square(5));
    printf("static_cube(3): %d\n", static_cube(3));
    printf("dynamic_double(7): %d\n", dynamic_double(7));
    printf("dynamic_negate(42): %d\n", dynamic_negate(42));
    return 0;
}