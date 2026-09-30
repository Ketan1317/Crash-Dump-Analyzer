#ifndef SYMBOL_RESOLVER_H
#define SYMBOL_RESOLVER_H

#include <elf.h>
#include <stddef.h>

char *find_symbol(Elf64_Sym *symbols, size_t symbol_count, char *symbol_names,
                  unsigned long address);

unsigned long get_load_base(void);
unsigned long runtime_to_elf_address(unsigned long runtime_address, unsigned long load_base);

int find_mapping(
    unsigned long runtime_address,
    char *path,
    size_t path_size,
    unsigned long *load_base
);

#endif