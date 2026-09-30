#include <stdio.h>

#include "crash_handler.h"
#include "elf_reader.h"

int main(void) {

  printf("Crash analyzer started.\n");

  SymbolTable symbol_table = {0};

  if (load_symbol_table("./crash-analyzer", &symbol_table) != 0) {
    printf("Failed to load symbol table.\n");
    return 1;
  }
  printf("Loaded %zu symbols.\n", symbol_table.symbol_count);

  install_crash_handler(&symbol_table);

  int *ptr = (int *)0x1234;
  printf("About to cause a crash...\n");
  *ptr = 42;

  free_symbol_table(&symbol_table);
  return 0;
}