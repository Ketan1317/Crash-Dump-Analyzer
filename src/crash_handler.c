#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <execinfo.h>
#include <ucontext.h>

#include "symbol_table.h"
#include <crash_handler.h>
#include <symbol_resolver.h>

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

// The siginfo_t structure is used to hold information about a signal
// static makes the function local to that source file.
static void crash_handler(int signal, siginfo_t *info, void *context) {
  ucontext_t *uc = (ucontext_t *)context;
  // ucontext_t is a structure provided by the system that represents execution
  // context. uc->uc_mcontext:  which contains the machine-specific CPU state.
  // gregs : is an array containing general-purpose register values.

  int fd = open("crash_report.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    printf("Could not create crash report!\n");
    exit(-1);
  }

  fprintf(stderr, "\nProgram crashed!\n");

  fprintf(stderr, "Signal: %d\n", signal);
  fprintf(stderr, "Fault address: %p\n",
          info->si_addr); // contains the address where the invalid memory
                          // access happened

  fprintf(stderr, "\nRegisters:\n");

  // RIP = Instruction Pointer Register
  // It tells the CPU where the next/current instruction is located.
  fprintf(stderr, "RIP = %p\n",
          (void *)uc->uc_mcontext.gregs[REG_RIP]); // Give me the value of RIP
                                                   // at the time of the crash.

  // RSP = Stack Pointer
  // It points into the current stack.
  fprintf(stderr, "RSP = %p\n", (void *)uc->uc_mcontext.gregs[REG_RSP]);

  // RBP = Base Pointer
  // Traditionally, it points to the current stack frame.
  fprintf(stderr, "RBP = %p\n", (void *)uc->uc_mcontext.gregs[REG_RBP]);

  fprintf(stderr, "RAX = %p\n", (void *)uc->uc_mcontext.gregs[REG_RAX]);
  fprintf(stderr, "RBX = %p\n", (void *)uc->uc_mcontext.gregs[REG_RBX]);
  fprintf(stderr, "RCX = %p\n", (void *)uc->uc_mcontext.gregs[REG_RCX]);
  fprintf(stderr, "RDX = %p\n", (void *)uc->uc_mcontext.gregs[REG_RDX]);

  unsigned long runtime_rip = (unsigned long)uc->uc_mcontext.gregs[REG_RIP];
  unsigned long load_base = get_load_base();
  unsigned long elf_address = runtime_to_elf_address(runtime_rip, load_base);

  // Resolve ELF address to function name.
  if (symbol_table != NULL && elf_address != 0) {

    char *function =
        find_symbol(symbol_table->symbols, symbol_table->symbol_count,
                    symbol_table->symbol_names, elf_address);

    if (function != NULL) {
      fprintf(stderr, "Function:     %s\n", function);
    } else {
      fprintf(stderr, "Function:     <unknown>\n");
    }
  } else {
    fprintf(stderr, "Function:     <unavailable>\n");
  }

  fprintf(stderr, "\nStack trace:\n");

  void *stack[20];

  // A stack frame is the portion of the call stack associated with one function
  // call, containing information needed for that particular invocation
  // ┌─────────────────────┐
  // │ foo() frame         │
  // │                     │
  // │ local variable y    │
  // │ parameter x         │
  // │ return information  │
  // │ saved registers     │
  // └─────────────────────┘
  int frame_count =
      backtrace(stack, 20); // "Walk the current call stack and give me the
                            // addresses of the frames."
  // stack trace - recorded list of active function calls or stack frames at
  // particular point in program execution It gives array of : 0x401250 0x4012c0

  // backtrace_symbols_fd() - asks the library to turn those addresses into
  // symbol information where possible and write the result to report file. A
  // symbol is basically a named entity associated with an address or other
  // object in the binary.

  // DWARF - So instead of: Crash at 0x40115c
  // we can report: Segmentation fault at main.c:6
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

        fprintf(
            stderr,
            "#%-2d  0x%lx  %s  base=0x%lx\n",
            i,
            runtime_address,
            path,
            base
        );

    } else {

        fprintf(
            stderr,
            "#%-2d  0x%lx  <no mapping>\n",
            i,
            runtime_address
        );
    }
}

  close(fd);
  fprintf(stderr, "Crash report generated: crash_report.txt\n");

  // ELF = Executable and Linkable Format is the structure Linux uses to
  // organize an executable and the information associated with it. .text
  // contains executable machine instructions. -g means: Generate debugging
  // information for use
  //  Position Independent Executable (PIE) is an ELF binary compiled as a
  //  shared object that allows OS to load the program into random memory
  //  addresses, enabling Address Space Layout Randomization (ASLR) for security
  //  hardening.
  // The executable is designed so that it can execute correctly regardless of
  // where the loader places it in memory.
  exit(EXIT_FAILURE);
}
// It is a callback defined by the OS interface and Linux calls it according to
// the required signature. So your handler receives information from the
// operating system. SIGSEGV (Signal 11) stands for Segmentation Violation

// If my process gets a SIGSEGV, don't handle it with the default behavior. Call
// my crash_handler() function and give it detailed information about what
// happened."
void install_crash_handler(SymbolTable *table) {
  symbol_table = table;
  struct sigaction action = {0}; // We create a configuration describing how we
                                 // want Linux to handle the signal

  action.sa_sigaction =
      crash_handler; // Call my crash_handler() when the signal happens.
  action.sa_flags = SA_SIGINFO; // I want the extended signal information
  // Without it, our handler wouldn't receive the useful siginfo_t information
  // in this form.

  // This initializes the signal mask for the handler.
  sigemptyset(&action.sa_mask); // Start with no additional signals blocked
                                // while this handler is executing.

  // registers that configuration with the kernel
  sigaction(SIGSEGV, &action, NULL); // telling the OS: "If this particular
                                     // signal happens, call this function."
  // NULL - It can be used to retrieve the previous signal configuration.
}