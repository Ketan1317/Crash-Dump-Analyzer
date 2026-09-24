# Crash Analyzer

## Project Description

A small crash analysis tool written in C for Linux. It captures information when a program crashes and generates a simple diagnostic report containing the signal, fault address, CPU registers, and stack information.

## Goal

The goal of this project is to understand what happens inside a program when it crashes and learn about Linux signals, process state, registers, stack frames, and debugging internals.

## Planned Features

* Detect common program crashes
* Capture crash signals
* Display the fault address
* Display CPU register values
* Generate a basic stack trace
* Produce a readable crash report

## Build

```bash
make
```

## Run

```bash
./crash-analyzer
```

## Technologies

* C
* Linux
* POSIX Signals
* Linux process and debugging APIs
* Make

## Project Structure

* `src/` — source files
* `include/` — header files
* `Makefile` — build configuration
