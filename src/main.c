#include <stdio.h>
#include <stdlib.h>

#include "crash_handler.h"
#include "elf_reader.h"

void write_to_memory(int *ptr) {
  *ptr = 42;
}

void process_data(int *ptr) {
  write_to_memory(ptr);
}

void analyze_data(void) {
  int *invalid_ptr = (int *)0x1234;

  process_data(invalid_ptr);
}

void run_analysis(void) {
  analyze_data();
}

int main(void) {
  printf("Crash analyzer started.........\n");

  SymbolTable symbol_table = {0};

  if (load_symbol_table("./crash-analyzer", &symbol_table) != 0) {
    printf("Failed to load symbol table!\n");
    exit(-1);
  }
  printf("Loaded %zu symbols.\n", symbol_table.symbol_count);
  
  install_crash_handler(&symbol_table);

  printf("About to cause a crash...\n");
  run_analysis();

  free_symbol_table(&symbol_table);
  return 0;
}