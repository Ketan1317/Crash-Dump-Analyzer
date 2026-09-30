#ifndef ELF_READER_H
#define ELF_READER_H

#include "symbol_table.h"

int load_symbol_table(char *filename, SymbolTable *table);

void free_symbol_table(SymbolTable *table);

#endif