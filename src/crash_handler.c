#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <ucontext.h>

#include <crash_handler.h>

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

// The siginfo_t structure is used to hold information about a signal
static void crash_handler(int signal, siginfo_t *info, void *context) {
    ucontext_t *uc = (ucontext_t *)context; // ucontext_t is a structure provided by the system that represents execution context.
    // uc->uc_mcontext:  which contains the machine-specific CPU state.
    // gregs : is an array containing general-purpose register values.

    fprintf(stderr, "\nProgram crashed!\n");

    fprintf(stderr, "Signal: %d\n", signal);
    fprintf(stderr, "Fault address: %p\n", info->si_addr); // contains the address where the invalid memory access happened

    fprintf(stderr, "\nRegisters:\n");

    // RIP = Instruction Pointer Register
    // It tells the CPU where the next/current instruction is located.
    fprintf(stderr, "RIP = %p\n",
            (void *)uc->uc_mcontext.gregs[REG_RIP]); // Give me the value of RIP at the time of the crash.

    // RSP = Stack Pointer
    // It points into the current stack.
    fprintf(stderr, "RSP = %p\n",
            (void *)uc->uc_mcontext.gregs[REG_RSP]);

    // RBP = Base Pointer
    // Traditionally, it points to the current stack frame.
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

    exit(EXIT_FAILURE);
}
// It is a callback defined by the OS interface and Linux calls it according to the required signature.
// So your handler receives information from the operating system.


// SIGSEGV (Signal 11) stands for Segmentation Violation
void install_crash_handler(void){
    struct sigaction action; // We create a configuration describing how we want Linux to handle the signal

    action.sa_sigaction = crash_handler; // Call my crash_handler() when the signal happens.
    action.sa_flags = SA_SIGINFO; // I want the extended signal information
    // Without it, our handler wouldn't receive the useful siginfo_t information in this form.

    // This initializes the signal mask for the handler.
    sigemptyset(&action.sa_mask); // Start with no additional signals blocked while this handler is executing.

    // registers that configuration with the kernel
    sigaction(SIGSEGV, &action, NULL); // telling the OS: "If this particular signal happens, call this function."

}