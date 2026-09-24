#include <stdio.h>

#include "crash_handler.h"
#include "elf_reader.h"

int main(void) {
    install_crash_handler();

    printf("Crash analyzer started.\n");

    inspect_elf("./crash-analyzer");

    int *ptr = (int *)0x1234;

    printf("About to cause a crash...\n");

    *ptr = 42;

    return 0;
}