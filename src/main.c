#include <stdio.h>

#include "crash_handler.h"

int main(void) {
    install_crash_handler();

    printf("Crash analyzer started.\n");

    int *ptr = NULL;

    printf("About to cause a crash...\n");

    *ptr = 42;

    return 0;
}