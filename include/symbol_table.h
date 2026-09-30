#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <elf.h>
#include <stddef.h>

typedef struct {
    Elf64_Sym *symbols;
    size_t symbol_count;
    char *symbol_names;
} SymbolTable;

#endif