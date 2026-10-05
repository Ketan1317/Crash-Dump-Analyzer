# Crash Dump Analyzer

A Linux-based crash analysis tool written in C that captures and analyzes information from a crashed process, including signals, fault addresses, CPU registers, stack frames, memory mappings, and function names.

## Overview

The project explores how crash-analysis tools work internally on Linux. It combines signal handling, `/proc` process information, ELF parsing, address translation, and symbol resolution to produce a structured crash report.

## Features

- Handles `SIGSEGV` using `sigaction`
- Captures fault address and CPU registers
- Generates stack traces using `backtrace()`
- Parses 64-bit ELF binaries
- Reads `.symtab` and its associated string table
- Resolves runtime addresses to function names
- Handles PIE and ASLR using the executable load base
- Reads `/proc/self/maps` to identify memory mappings
- Identifies shared-library frames
- Generates `crash_report.txt`

## How It Works

```text
Program Crash
     ↓
   SIGSEGV
     ↓
Crash Handler
     ├── Signal + Fault Address
     ├── CPU Registers
     └── Stack Trace
             ↓
      /proc/self/maps
             ↓
       Load Base
             ↓
Runtime Address - Load Base
             ↓
       ELF Address
             ↓
      Symbol Table
             ↓
      Function Name
             ↓
     Crash Report
```

For PIE executables, runtime addresses differ from the addresses stored in the ELF file because of ASLR.

```text
ELF Address = Runtime Address - Load Base
```

## Project Structure

```text
Crash Dump Analyzer/
├── src/
│   ├── main.c
│   ├── crash_handler.c
│   ├── elf_reader.c
│   └── symbol_resolver.c
├── include/
│   ├── crash_handler.h
│   ├── elf_reader.h
│   ├── symbol_resolver.h
│   └── symbol_table.h
├── Makefile
├── .gitignore
├── LICENSE
└── README.md
```

### Components

- **`main.c`** — Initializes the analyzer and contains the controlled crash used for testing.
- **`crash_handler.c`** — Handles `SIGSEGV`, reads CPU context, collects stack frames, and generates the crash report.
- **`elf_reader.c`** — Reads the ELF executable and loads its symbol table.
- **`symbol_resolver.c`** — Handles load-base detection, `/proc/self/maps`, address translation, and function lookup.
- **`symbol_table.h`** — Defines the in-memory symbol table representation.

## Requirements

- Linux or WSL2
- GCC
- GNU Make
- x86-64 system

The project uses Linux-specific facilities including:

- `sigaction`
- `siginfo_t`
- `ucontext_t`
- `/proc/self/maps`
- `/proc/self/exe`
- `execinfo`
- ELF structures from `<elf.h>`

## Build

Clone the repository:

```bash
git clone https://github.com/Ketan1317/Crash-Dump-Analyzer.git
cd "Crash Dump Analyzer"
```

Build:

```bash
make
```

Clean:

```bash
make clean
```

The Makefile builds with `-g` so the executable contains debugging information.

## Usage

Run:

```bash
./crash-analyzer
```

The current test program intentionally triggers a segmentation fault through this call chain:

```text
main
 ↓
run_analysis
 ↓
analyze_data
 ↓
process_data
 ↓
write_to_memory
 ↓
SIGSEGV
```

A crash report is written to:

```text
crash_report.txt
```

Example stack trace:

```text
#0   0x19a2  crash_handler
#1   0x45cb0  /usr/lib/x86_64-linux-gnu/libc.so.6  <unknown>
#2   0x1419  write_to_memory
#3   0x143e  process_data
#4   0x1461  analyze_data
#5   0x1471  run_analysis
#6   0x1519  main
#7   0x2a601  /usr/lib/x86_64-linux-gnu/libc.so.6  <unknown>
#8   0x2a718  /usr/lib/x86_64-linux-gnu/libc.so.6  <unknown>
#9   0x1345  _start
```

Runtime addresses vary between executions because of ASLR.

## Crash Report

The generated report contains:

```text
Process Information
------------------------------------------------------------
Signal          : 11
Fault Address   : 0x1234

CPU Registers
------------------------------------------------------------
RIP             : 0x600f0a8b7419
RSP             : 0x7ffe11d19858
RBP             : 0x7ffe11d19858
RAX             : 0x1234

Crash Location
------------------------------------------------------------
Runtime Address : 0x600f0a8b7419
Load Base       : 0x600f0a8b6000
ELF Address     : 0x1419
Function        : write_to_memory

Stack Trace
------------------------------------------------------------
#2   0x1419  write_to_memory
#3   0x143e  process_data
#4   0x1461  analyze_data
#5   0x1471  run_analysis
#6   0x1519  main
```

## Technical Details

### ELF Symbol Resolution

The analyzer reads:

- ELF header (`Elf64_Ehdr`)
- Section headers (`Elf64_Shdr`)
- Symbol entries (`Elf64_Sym`)
- `.symtab`
- Associated string table

Function symbols are identified using:

```c
ELF64_ST_TYPE(symbol.st_info) == STT_FUNC
```

A symbol is matched when the address falls within:

```text
st_value <= address < st_value + st_size
```

### PIE / ASLR

The executable's runtime mapping is obtained from:

```text
/proc/self/maps
```

The load base is used to convert a runtime virtual address into the corresponding ELF-relative address.

### Stack Frames

`backtrace()` provides runtime stack addresses. Each address is checked against `/proc/self/maps` to determine the mapped executable or shared library.

The current implementation resolves symbols from the main executable. Shared-library mappings such as `libc.so.6` are identified, but their symbols are not currently loaded.

## Limitations

- Currently focuses on `SIGSEGV`.
- Shared-library function symbols are not resolved.
- DWARF parsing and source-line information are not implemented.
- Core dump analysis is not implemented.
- Stack unwinding relies on `backtrace()`.
- The signal handler uses functions that are not all async-signal-safe, so this should be considered a learning/project implementation rather than a production crash handler.

## Future Improvements

- Shared-library symbol resolution
- DWARF-based source and line information
- Support for additional crash signals
- Custom stack unwinding
- Core dump analysis
- Improved async-signal-safe crash handling
