#include <elf.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "symbol_resolver.h"

char *find_symbol(Elf64_Sym *symbols, size_t symbol_count, char *symbol_names,
                  unsigned long address) {

  for (size_t i = 0; i < symbol_count; i++) {
    if (ELF64_ST_TYPE(symbols[i].st_info) != STT_FUNC) {
      continue;
    }

    unsigned long start = symbols[i].st_value;
    unsigned long end = start + symbols[i].st_size;

    // Check whether the address lies inside this function
    if (address >= start && address < end) {
      return symbol_names + symbols[i].st_name;
    }
  }
  return NULL;
}

// where my executable is loaded in memory while the program is running
unsigned long get_load_base(void) {
  // Linux exposes the current process's memory layout at: /proc/self/maps

  // Address range                  Purpose
  // 55a8c4f00000 ─ 55a8c4f01000   executable's read-only area
  // 55a8c4f01000 ─ 55a8c4f02000   executable's code
  // 55a8c4f02000 ─ 55a8c4f03000   executable's writable data
  int fd = open("/proc/self/maps", O_RDONLY);

  if (fd < 0) {
    perror("open /proc/self/maps");
    return 0;
  }

  char buffer[8192];
  ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

  if (bytes_read <= 0) {
    perror("read /proc/self/maps");
    close(fd);
    return 0;
  }
  buffer[bytes_read] = '\0';

  //  Get the path of our executable.
  char executable_path[512];
  // Linux provides a special symbolic link: /proc/self/exe
  // It points to the currently running executable
  ssize_t path_length =
      readlink("/proc/self/exe", executable_path, sizeof(executable_path) - 1);

  // The readlink() function is used to read the contents of a symbolic link
  // into a buffer
  if (path_length < 0) {
    perror("readlink");
    close(fd);
    return 0;
  }

  executable_path[path_length] = '\0';

  // Parse /proc/self/maps line by line

  char *line = buffer;
  while (line < buffer + bytes_read) {
    char *newline = strchr(line, '\n');
    if (newline != NULL) {
      // sscanf() expect a null-terminated string.
      *newline = '\0';
    }

    // For example:
    // 55a8c4f00000-55a8c4f01000 r--p 00000000 08:01 12345
    // /home/user/crash-analyzer

    // We want:
    // start       = 55a8c4f00000
    // end         = 55a8c4f01000
    // permissions = r--p
    // path        = /home/user/crash-analyzer
    unsigned long start;
    unsigned long end;
    unsigned long offset;

    char permissions[5];
    char device[20];
    unsigned long inode;

    // Think of sscanf() as: scanf() but instead of reading from keyboard, read
    // from a string.
    int result = sscanf(line, "%lx-%lx %4s %lx %19s %lu", &start, &end,
                        permissions, &offset, device, &inode);

    // sscanf() returns the number of fields it successfully assigned
    if (result == 6) {
      char *path = line;
      for (int i = 0; i < 5; i++) {
        path = strchr(path, ' ');

        if (path == NULL) {
          break;
        }

        while (*path == ' ') {
          path++;
        }
      }
      if (path != NULL && offset == 0 && strcmp(path, executable_path) == 0) {
        close(fd);
        return start;
      }
    }

    if (newline == NULL) {
      break;
    }
    line = newline + 1;
  }
  close(fd);

  return 0;
}

unsigned long runtime_to_elf_address(unsigned long runtime_address,
                                     unsigned long load_base) {
  if (runtime_address < load_base) {
    return 0;
  }
  return runtime_address - load_base;
}

int find_mapping(unsigned long runtime_address, char *path, size_t path_size,
                 unsigned long *load_base) {

  int fd = open("/proc/self/maps", O_RDONLY);
  if (fd < 0) {
    perror("open /proc/self/maps");
    return -1;
  }

  char buffer[65536];
  ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);

  if (bytes_read <= 0) {
    close(fd);
    return -1;
  }

  buffer[bytes_read] = '\0';

  char *line = buffer;
  while (line < buffer + bytes_read) {
    char *newline = strchr(line, '\n');

    if (newline != NULL) {
      *newline = '\0';
    }

    unsigned long start;
    unsigned long end;
    unsigned long offset;
    unsigned long inode;

    char permissions[5];
    char device[20];

    int result = sscanf(line, "%lx-%lx %4s %lx %19s %lu", &start, &end,
                        permissions, &offset, device, &inode);

    if (result == 6) {
      if (runtime_address >= start && runtime_address < end) {
        char *mapped_path = line;

        for (int i = 0; i < 5; i++) {
          mapped_path = strchr(mapped_path, ' ');

          if (mapped_path == NULL) {
            break;
          }

          while (*mapped_path == ' ') {
            mapped_path++;
          }
        }

        if (mapped_path != NULL && *mapped_path != '\0') {
          size_t length = strlen(mapped_path);

          if (length >= path_size) {
            length = path_size - 1;
          }

          memcpy(path, mapped_path, length);
          path[length] = '\0';
        } else {
          path[0] = '\0';
        }

        *load_base = start - offset;

        close(fd);
        return 0;
      }
    }

    if (newline == NULL) {
      break;
    }

    line = newline + 1;
  }

  close(fd);
  return -1;
}
