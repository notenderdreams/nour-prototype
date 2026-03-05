#include <stdio.h>
#include "another.h"

int main() {
    printf("Hello, World!\n");
    int result = another_function();
    printf("Result from another_function: %d\n", result); 
    return 0;
}