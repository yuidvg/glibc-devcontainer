# Memory Allocator Project Summary

## Project Overview

This project implements a custom memory allocator that replaces the standard `malloc`, `free`, and `realloc` functions in C. The allocator uses a zone-based approach to efficiently manage memory for different allocation sizes, with specialized handling for tiny, small, and large allocations.

## Core Components

### 1. Memory Management Strategy

The allocator divides memory into three categories:

- **Tiny allocations**: <= 128 bytes
- **Small allocations**: > 128 bytes and <= 1024 bytes
- **Large allocations**: > 1024 bytes

Each category has its own memory zone management strategy:

- Tiny allocations use a pre-allocated zone of 16 pages
- Small allocations use a pre-allocated zone of 128 pages
- Large allocations are mapped directly using mmap/munmap

### 2. Key Data Structures

- `ChunkHeader`: Metadata for memory blocks (size, free status)
- `LargeChunkHeader`: Metadata for large allocations with additional tracking
- `Zone`: Management structure for memory zones
- `AllocResult`: Result type for allocation functions

### 3. Memory Management Techniques

- Alignment-aware memory allocation
- Constructor/destructor for zone initialization and cleanup
- Block splitting for efficient memory use
- Support for reallocation with optimized in-place resizing when possible

### 4. API Implementation

The allocator implements the standard memory allocation API:
- `void *malloc(size_t size)`
- `void free(void *ptr)`
- `void *realloc(void *ptr, size_t size)`

Additional utility functions:
- `show_alloc_mem()`: Displays allocation information for debugging

## Project Structure

```
/
├── include/               # Public API headers
│   └── malloc.h           # Standard malloc replacement API
├── src/                   # Implementation source code
│   ├── all.h              # Main include for implementation files
│   ├── defines.h          # Core data structures and constants
│   ├── external.h         # External dependencies
│   ├── prototypes.h       # Internal function prototypes
│   ├── ftMalloc.c         # Main malloc implementation
│   ├── ftFree.c           # Free implementation
│   ├── ftRealloc.c        # Realloc implementation
│   ├── initialization.c   # Zone initialization logic
│   ├── publish.c          # Public API symbol exports
│   ├── showAllocMem.c     # Memory usage reporting
│   ├── syscalls.c         # System call wrappers
│   └── utils.c            # Utility functions
├── test/                  # Testing framework
│   ├── munit.c            # µnit test framework implementation
│   ├── munit.h            # µnit test framework header
│   ├── test.h             # Common test utilities
│   └── testInitialization.c # Tests for initialization and basic allocation
├── debug/                 # Debug utilities
├── libft/                 # Support library for string and memory operations
├── build/                 # Build outputs
│   ├── bin/               # Binary outputs including shared library
│   └── obj/               # Object files
└── Makefile               # Build system
```

## Implementation Details

1. **Functional Style C**
   - Uses immutable data where possible
   - Functions accept all dependencies as parameters
   - Uses result types like `AllocResult` for error handling

2. **Memory Allocation Strategy**
   - Pre-allocated zones for tiny and small allocations
   - Large allocations handled directly through system calls
   - Alignment handling for optimized memory access

3. **Testing**
   - Uses the µnit testing framework
   - Tests cover tiny, small, and large allocations
   - Memory integrity verification

## Build and Test

The project can be built and tested using the following commands:

```bash
# Build everything (malloc library and tests)
make all

# Build only the malloc library
make malloc

# Run tests
make run-test

# Clean build files
make clean

# Rebuild everything
make re
```

The memory allocator is built as a shared library that can be linked against applications, replacing the standard memory allocation functions at runtime.
