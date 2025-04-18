# Custom Memory Allocator

A custom memory allocator implementation with zone-based memory management, inspired by glibc's malloc.

## Features

- Zone-based memory allocation strategy
- Efficient handling of different allocation sizes
- Memory leak detection and reporting
- Best-fit allocation algorithm
- Memory coalescing to reduce fragmentation
- Functional programming approach in C

## Documentation

Detailed documentation is available in the `docs/` directory:

- [Project Summary](docs/project_summary.md) - Overview of the project
- [Technical Details](docs/technical_details.md) - Implementation details
- [API Reference](docs/api_reference.md) - Public API documentation
- [Functional Approach](docs/functional_approach.md) - Functional programming concepts used
- [Changelog](docs/changelog.md) - Recent changes and updates

## Build and Usage

Build the project using the provided Makefile:

```bash
# Build everything
make all

# Run tests
make test

# Run demo program
make run

# Clean build files
make clean
```

## Example

```c
#include "include/myMalloc.h"

int main(void) {
    // Allocate memory
    void* ptr = myMalloc(100);

    // Use the memory
    if (ptr) {
        // ... do something with the memory

        // Free the memory when done
        myFree(ptr);
    }

    // Check for memory leaks
    printMemoryLeakReport();

    return 0;
}
```
