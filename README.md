# Crash-Dump-Analyzer

A lightweight Linux crash analysis tool written in C. It captures information from a crashed process and inspects the executable's ELF structures to help understand what happened at the time of failure.

## Features

* Linux signal handling with `sigaction`
* Fault address and CPU register capture
* Basic stack trace generation
* Crash report generation
* ELF header and section parsing
* Symbol table and string table parsing
* Function symbol inspection

## Tech Stack

* C
* Linux / POSIX
* ELF
* Make
* GCC

## Build

```bash
make
```

## Run

```bash
./crash-analyzer
```

The project is built to understand the low-level concepts behind crash analysis, debugging, ELF binaries, process state, and stack traces.
