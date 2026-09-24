#include <elf.h>

// Elf64_Ehdr -  ELF header
//     ↓
// "What kind of ELF file is this and where can I find its other information?"

// Elf64_Shdr -  Section Header
//     ↓
// "Where is each section inside the ELF file?"

// Elf64_Sym - Symbol entry
//     ↓
// "What symbol exists at what address?"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "elf_reader.h"

void inspect_elf(char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }

  Elf64_Ehdr header;

  ssize_t bytes_read = read(fd, &header, sizeof(header));

  if (bytes_read != sizeof(header)) {
    printf("error in reading ELF header\n");
    close(fd);
    exit(-1);
  }

  printf("ELF file: %s\n", filename);

  printf("Entry point: 0x%lx\n", header.e_entry);

  printf("Section header offset: %lu\n",
         header.e_shoff); // where the section-header table is located inside
                          // the ELF file

  printf("Number of sections: %u\n", header.e_shnum);

  printf("Section header size: %u\n", header.e_shentsize);

  Elf64_Shdr *sections = malloc(header.e_shnum * sizeof(Elf64_Shdr));
  if (sections == NULL) {
    perror("malloc");
    close(fd);
    exit(EXIT_FAILURE);
  }

  lseek(fd, header.e_shoff, SEEK_SET);

  size_t section_table_size = header.e_shnum * sizeof(Elf64_Shdr);

  ssize_t section_bytes = read(fd, sections, section_table_size);

  if (section_bytes != (ssize_t)section_table_size) {
    perror("read section headers");
    free(sections);
    close(fd);
    exit(EXIT_FAILURE);
  }

  // header.e_shstrndx gives Which section-header entry contains the
  // section-name string table?
  Elf64_Shdr shstrtab_header = sections[header.e_shstrndx]; // e_shstrndx = 36
  char *section_names = malloc(shstrtab_header.sh_size);

  if (section_names == NULL) {
    perror("malloc");
    free(sections);
    close(fd);
    exit(EXIT_FAILURE);
  }

  lseek(fd, shstrtab_header.sh_offset, SEEK_SET);
  read(fd, section_names, shstrtab_header.sh_size);

  printf("\nSections:\n");

  for (int i = 0; i < header.e_shnum; i++) {
    printf("[%2d] %s\n", i, section_names + sections[i].sh_name);
  }

  int symtab_idx = -1;
  for (int i = 0; i < header.e_shnum; i++) {
    char *name = section_names + sections[i].sh_name;
    if (strcmp(name, ".symtab") == 0) {
      symtab_idx = i;
      break;
    }
  }

  if (symtab_idx == -1) {
    printf("Symbol table not found\n");

    free(section_names);
    free(sections);
    close(fd);
    return;
  }

  printf("\nSymbol table found at section: %d\n", symtab_idx);

  Elf64_Shdr symtab = sections[symtab_idx];
  int strtab_index = symtab.sh_link;
  Elf64_Shdr strtab = sections[strtab_index];

  printf("Symbol string table section: %d\n", strtab_index);

  // No. of symbols
  size_t symbol_count = symtab.sh_size / symtab.sh_entsize;

  char *symbol_names = malloc(strtab.sh_size);

  if (symbol_names == NULL) {
    perror("malloc");

    free(section_names);
    free(sections);
    close(fd);

    exit(EXIT_FAILURE);
  }

  lseek(fd, strtab.sh_offset, SEEK_SET);

  ssize_t string_bytes = read(fd, symbol_names, strtab.sh_size);

  if (string_bytes != (ssize_t)strtab.sh_size) {

    perror("read symbol string table");

    free(symbol_names);
    free(section_names);
    free(sections);
    close(fd);

    exit(EXIT_FAILURE);
  }

  Elf64_Sym *symbols = malloc(symtab.sh_size);

  if (symbols == NULL) {
    perror("malloc");

    free(section_names);
    free(sections);
    close(fd);

    exit(EXIT_FAILURE);
  }
  lseek(fd, symtab.sh_offset, SEEK_SET);

  ssize_t symbol_bytes = read(fd, symbols, symtab.sh_size);

  if (symbol_bytes != (ssize_t)symtab.sh_size) {
    perror("read symbol table");

    free(symbols);
    free(symbol_names);
    free(section_names);
    free(sections);
    close(fd);

    exit(EXIT_FAILURE);
  }

  printf("\nFunction symbols:\n");

  for (size_t i = 0; i < symbol_count; i++) {
    if (ELF64_ST_TYPE(symbols[i].st_info) == STT_FUNC) {
      printf("0x%lx  %s\n", symbols[i].st_value,
             symbol_names + symbols[i].st_name);
    }
  }

  free(symbols);
  free(symbol_names);
  free(section_names);
  free(sections);
  close(fd);
}
