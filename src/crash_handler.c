#define _GNU_SOURCE // "Expose GNU-specific and additional POSIX/Linux
                    // functionality when compiling this source file"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <execinfo.h>
#include <ucontext.h>

#include "crash_handler.h"
#include "symbol_resolver.h"
#include "symbol_table.h"

static SymbolTable *symbol_table = NULL;

// static makes the function local to this source file.
static void crash_handler(int signal, siginfo_t *info, void *context) {
  // The siginfo_t structure is used to hold information about a signal.

  ucontext_t *uc = (ucontext_t *)context;
  // ucontext_t represents the execution context at the time of the crash.
  // uc_mcontext contains the machine-specific CPU state.
  // gregs contains the general-purpose register values.

  int fd = open("crash_report.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

  if (fd < 0) {
    printf("Could not create crash report!\n");
    exit(EXIT_FAILURE);
  }

  dprintf(fd, "Process Information\n");
  dprintf(fd, "------------------------------------------------------------\n");
  dprintf(fd, "Signal          : %d\n", signal);
  dprintf(fd, "Fault Address   : %p\n\n", info->si_addr);

  dprintf(fd, "CPU Registers\n");
  dprintf(fd, "------------------------------------------------------------\n");

  dprintf(fd, "RIP             : %p\n", (void *)uc->uc_mcontext.gregs[REG_RIP]);
  dprintf(fd, "RSP             : %p\n", (void *)uc->uc_mcontext.gregs[REG_RSP]);
  dprintf(fd, "RBP             : %p\n", (void *)uc->uc_mcontext.gregs[REG_RBP]);
  dprintf(fd, "RAX             : %p\n", (void *)uc->uc_mcontext.gregs[REG_RAX]);
  dprintf(fd, "RBX             : %p\n", (void *)uc->uc_mcontext.gregs[REG_RBX]);
  dprintf(fd, "RCX             : %p\n", (void *)uc->uc_mcontext.gregs[REG_RCX]);
  dprintf(fd, "RDX             : %p\n\n",
          (void *)uc->uc_mcontext.gregs[REG_RDX]);

  unsigned long runtime_rip = (unsigned long)uc->uc_mcontext.gregs[REG_RIP];
  unsigned long load_base = get_load_base();

  unsigned long elf_address = runtime_to_elf_address(runtime_rip, load_base);

  dprintf(fd, "Crash Location\n");
  dprintf(fd, "------------------------------------------------------------\n");

  dprintf(fd, "Runtime Address : 0x%lx\n", runtime_rip);
  dprintf(fd, "Load Base       : 0x%lx\n", load_base);
  dprintf(fd, "ELF Address     : 0x%lx\n", elf_address);

  if (symbol_table != NULL && elf_address != 0) {

    char *function =
        find_symbol(symbol_table->symbols, symbol_table->symbol_count,
                    symbol_table->symbol_names, elf_address);

    if (function != NULL) {
      dprintf(fd, "Function        : %s\n\n", function);
    } else {
      dprintf(fd, "Function        : <unknown>\n\n");
    }

  } else {
    dprintf(fd, "Function        : <unavailable>\n\n");
  }

  dprintf(fd, "Stack Trace\n");
  dprintf(fd, "------------------------------------------------------------\n");

  void *stack[20];
  int frame_count = backtrace(stack, 20);

  // Resolve every stack address to its mapped ELF object.
  for (int i = 0; i < frame_count; i++) {
    unsigned long runtime_address = (unsigned long)stack[i];
    char path[512];
    unsigned long base;

    if (find_mapping(runtime_address, path, sizeof(path), &base) == 0) {
      unsigned long frame_elf_address = runtime_address - base;
      char *function = NULL;

      if (symbol_table != NULL && base == load_base) {
        function =
            find_symbol(symbol_table->symbols, symbol_table->symbol_count,
                        symbol_table->symbol_names, frame_elf_address);
      }

      if (function != NULL) {
        dprintf(fd, "#%-2d  0x%lx  %s\n", i, frame_elf_address, function);
      } else {
        dprintf(fd, "#%-2d  0x%lx  %s  <unknown>\n", i, frame_elf_address,
                path);
      }
    } else {
      dprintf(fd, "#%-2d  0x%lx  <no mapping>\n", i, runtime_address);
    }
  }

  close(fd);
  fprintf(stderr, "Crash report generated: crash_report.txt\n");

  exit(EXIT_FAILURE);
}

// If the process receives SIGSEGV, Linux calls crash_handler()
// with signal information and the current execution context.
void install_crash_handler(SymbolTable *table) {

  symbol_table = table;

  // Create a configuration describing how Linux should handle the signal.
  struct sigaction action = {0};

  // Call crash_handler() when SIGSEGV occurs.
  action.sa_sigaction = crash_handler;

  // Request extended signal information through siginfo_t.
  action.sa_flags = SA_SIGINFO;

  // Start with no additional signals blocked while the handler executes.
  sigemptyset(&action.sa_mask);

  // Register the signal handler with the kernel.
  //
  // The final NULL means we are not requesting the previous
  // signal configuration.
  sigaction(SIGSEGV, &action, NULL);
}