#include <stdio.h>
#include "another.h"
#include "boxbox.h"
#include "math/math_ops.h"
#include "utils/strutil.h"

int main() {
    greet("nour");
    printf("another_function: %d\n", another_function());
    printf("add(3, 4): %d\n", add(3, 4));
    printf("multiply(3, 4): %d\n", multiply(3, 4));
    return 0;
}