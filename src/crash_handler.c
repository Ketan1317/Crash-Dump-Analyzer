#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <execinfo.h>
#include <ucontext.h>

#include "symbol_table.h"
#include "crash_handler.h"
#include "symbol_resolver.h"

// context
//    │
//    ▼
// ┌──────────────────────────┐
// │ ucontext_t               │
// │                          │
// │ machine context          │
// │                          │
// │ registers                │
// │ stack information        │
// │ signal information       │
// └──────────────────────────┘

static SymbolTable *symbol_table = NULL;

// The siginfo_t structure is used to hold information about a signal.
// static makes the function local to this source file.
static void crash_handler(int signal, siginfo_t *info, void *context) {

  ucontext_t *uc = (ucontext_t *)context;

  // ucontext_t represents the execution context at the time of the crash.
  // uc_mcontext contains the machine-specific CPU state.
  // gregs contains the general-purpose register values.

  int fd = open(
      "crash_report.txt",
      O_WRONLY | O_CREAT | O_TRUNC,
      0644
  );

  if (fd < 0) {
    printf("Could not create crash report!\n");
    exit(EXIT_FAILURE);
  }

  /*
   * Crash report header.
   */
  dprintf(fd,
          "============================================================\n"
          "                    CRASH DUMP REPORT\n"
          "============================================================\n\n");

  dprintf(fd, "Process Information\n");
  dprintf(fd,
          "------------------------------------------------------------\n");

  dprintf(fd, "Signal          : %d\n", signal);
  dprintf(fd, "Fault Address   : %p\n\n", info->si_addr);

  /*
   * Display the crash information in the terminal as well.
   */
  fprintf(stderr, "\nProgram crashed!\n");
  fprintf(stderr, "Signal: %d\n", signal);
  fprintf(stderr, "Fault address: %p\n", info->si_addr);

  /*
   * CPU registers.
   *
   * RIP = Instruction Pointer
   * RSP = Stack Pointer
   * RBP = Base Pointer
   */
  fprintf(stderr, "\nRegisters:\n");

  fprintf(stderr, "RIP = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RIP]);

  fprintf(stderr, "RSP = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RSP]);

  fprintf(stderr, "RBP = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RBP]);

  fprintf(stderr, "RAX = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RAX]);

  fprintf(stderr, "RBX = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RBX]);

  fprintf(stderr, "RCX = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RCX]);

  fprintf(stderr, "RDX = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RDX]);

  /*
   * Write registers to the crash report.
   */
  dprintf(fd, "CPU Registers\n");
  dprintf(fd,
          "------------------------------------------------------------\n");

  dprintf(fd, "RIP             : %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RIP]);

  dprintf(fd, "RSP             : %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RSP]);

  dprintf(fd, "RBP             : %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RBP]);

  dprintf(fd, "RAX             : %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RAX]);

  dprintf(fd, "RBX             : %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RBX]);

  dprintf(fd, "RCX             : %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RCX]);

  dprintf(fd, "RDX             : %p\n\n",
          (void *)uc->uc_mcontext.gregs[REG_RDX]);

  /*
   * Resolve the instruction pointer.
   *
   * Runtime address
   *       ↓
   * Load base
   *       ↓
   * ELF-relative address
   *       ↓
   * Symbol table
   *       ↓
   * Function name
   */
  unsigned long runtime_rip =
      (unsigned long)uc->uc_mcontext.gregs[REG_RIP];

  unsigned long load_base =
      get_load_base();

  unsigned long elf_address =
      runtime_to_elf_address(
          runtime_rip,
          load_base
      );

  fprintf(stderr, "Function:     ");

  dprintf(fd, "Crash Location\n");
  dprintf(fd,
          "------------------------------------------------------------\n");

  dprintf(fd, "Runtime Address : 0x%lx\n", runtime_rip);
  dprintf(fd, "Load Base       : 0x%lx\n", load_base);
  dprintf(fd, "ELF Address     : 0x%lx\n", elf_address);

  /*
   * Resolve ELF address to function name.
   */
  if (symbol_table != NULL && elf_address != 0) {

    char *function =
        find_symbol(
            symbol_table->symbols,
            symbol_table->symbol_count,
            symbol_table->symbol_names,
            elf_address
        );

    if (function != NULL) {

      fprintf(stderr, "%s\n", function);
      dprintf(fd, "Function        : %s\n\n", function);

    } else {

      fprintf(stderr, "<unknown>\n");
      dprintf(fd, "Function        : <unknown>\n\n");
    }

  } else {

    fprintf(stderr, "<unavailable>\n");
    dprintf(fd, "Function        : <unavailable>\n\n");
  }

  /*
   * Generate stack trace.
   *
   * backtrace() walks the current call stack and stores
   * the addresses of the stack frames.
   */
  fprintf(stderr, "\nStack trace:\n");

  dprintf(fd, "Stack Trace\n");
  dprintf(fd,
          "------------------------------------------------------------\n");

  void *stack[20];

  int frame_count =
      backtrace(stack, 20);

  /*
   * Resolve every stack address to its mapped ELF object.
   */
  for (int i = 0; i < frame_count; i++) {

    unsigned long runtime_address =
        (unsigned long)stack[i];

    char path[512];

    unsigned long base;

    if (find_mapping(
            runtime_address,
            path,
            sizeof(path),
            &base) == 0) {

      /*
       * Terminal output.
       */
      fprintf(
          stderr,
          "#%-2d  0x%lx  %s  base=0x%lx\n",
          i,
          runtime_address,
          path,
          base
      );

      /*
       * Crash report output.
       */
      dprintf(
          fd,
          "#%-2d  0x%lx  %s\n",
          i,
          runtime_address,
          path
      );

      dprintf(
          fd,
          "      Load Base: 0x%lx\n",
          base
      );

    } else {

      fprintf(
          stderr,
          "#%-2d  0x%lx  <no mapping>\n",
          i,
          runtime_address
      );

      dprintf(
          fd,
          "#%-2d  0x%lx  <no mapping>\n",
          i,
          runtime_address
      );
    }
  }

  /*
   * Finish the report.
   */
  dprintf(fd,
          "\n============================================================\n"
          "                    END OF REPORT\n"
          "============================================================\n");

  close(fd);

  fprintf(
      stderr,
      "Crash report generated: crash_report.txt\n"
  );

  /*
   * ELF = Executable and Linkable Format.
   *
   * PIE allows the executable to be loaded at different
   * virtual addresses. ASLR randomizes that location.
   */
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