# Its job is to tell the make tool how to compile and link your C project

CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude

# -Wall -Wextra -- Enable common compiler warnings, Give me useful warnings about potentially problematic code
# -g -- includes debugging information in the executable.
# -Iinclude -- tells GCC, Also look inside the include/ directory when searching for header files.

TARGET = crash-analyzer

SRC = src/main.c \
      src/crash_handler.c

OBJ = $(SRC:.c=.o)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) 
# 	GCC's linker combines them

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)


# .o (Object File): This is an intermediate relocatable binary file produced by the compiler.  It contains machine code but lacks the necessary linking to specific memory addresses and libraries. It cannot be executed directly; it must be linked with other object files and libraries to become an executable. 
# .out (Executable File): This is the final executable image (often named a.out by default on Unix-like systems).  It is fully linked, meaning all symbols are resolved to actual memory locations and libraries are attached. It is ready to be run by the operating system. 