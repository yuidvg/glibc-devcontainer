# Custom Memory Allocator

A custom memory allocator implementation in C, inspired by glibc's malloc, leveraging zone-based memory management and designed with functional programming principles.

## Features

- **Zone-based allocation**: Efficiently manages `TINY`, `SMALL`, and `LARGE` allocations.
- **Pre-allocated memory pools**: Uses `mmap` to pre-allocate memory regions for `TINY` and `SMALL` zones.
- **Best-fit algorithm**: Employs a best-fit strategy for allocating within zones.
- **Memory coalescing**: Merges adjacent free blocks to reduce fragmentation.
- **Standard interface**: Provides `malloc`, `free`, and `realloc` overrides.
- **Memory visualization**: Includes `show_alloc_mem` to display the state of allocated memory.
- **Functional Style**: Adheres to functional C guidelines (immutability where possible, pure internal functions, explicit state management).
- **Robust Testing**: Includes a comprehensive test suite using MUnit.

## Documentation

Detailed documentation is available in the `doc/` directory:

- [Project Summary](docs/project_summary.md) - Overview of the project
- [Technical Details](docs/technical_details.md) - Implementation details (Placeholder/Needs Update)
- [API Reference](docs/api_reference.md) - Public API documentation (Placeholder/Needs Update)
- [Functional Approach](docs/functional_approach.md) - Functional programming concepts used (Placeholder/Needs Update)
- [Changelog](docs/changelog.md) - Recent changes and updates

## Build and Usage

Build the project using the provided Makefile:

```bash
# Build the shared library and test executable
make all

# Run tests
make run-test

# Build and run the debug executable (if debug/main.c exists)
# make debug
# ./build/bin/debug.out

# Clean build files
make clean

# Rebuild everything
make re
```

### Using the Library

To use the custom allocator in your project, link against the generated shared library (`libft_malloc.so`):

1.  **Build the library**: `make malloc` (or `make all`)
2.  **Compile your code**: `gcc your_code.c -Iinclude -Lbuild/bin -lft_malloc -Wl,-rpath=./build/bin -o your_program`
3.  **Run**: `./your_program`

The library overrides the standard `malloc`, `free`, and `realloc` functions.

## Example

```c
#include "include/malloc.h" // Use the custom allocator header
#include <stdio.h>
#include <string.h>

int main(void) {
    // Allocate memory using the custom malloc
    char *str = (char *)malloc(100 * sizeof(char));

    if (str == NULL) {
        perror("Malloc failed");
        return 1;
    }

    // Use the memory
    strcpy(str, "Hello from custom malloc!");
    printf("Allocated string: %s\n", str);

    // Show memory layout (optional)
    printf("\n--- Memory Layout Before Free ---\n");
    show_alloc_mem();

    // Reallocate memory (example)
    char *new_str = (char *)realloc(str, 200 * sizeof(char));
    if (new_str == NULL) {
        perror("Realloc failed");
        free(str); // Free original block if realloc fails
        return 1;
    }
    str = new_str; // Update pointer
    strcat(str, " Now reallocated!");
    printf("Reallocated string: %s\n", str);

    // Free the memory when done using the custom free
    free(str);

    printf("\n--- Memory Layout After Free ---\n");
    show_alloc_mem(); // Show layout after freeing

    return 0;
}
```
