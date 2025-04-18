# Memory Allocator Project Summary

## Project Overview

This project implements a custom memory allocator inspired by the `malloc` and `free` functions in C. The allocator uses a zone-based approach to efficiently manage memory for different allocation sizes, with specialized handling for tiny, small, and large allocations.

## Core Components

### 1. Memory Management Strategy

The allocator divides memory into three categories:

- **Tiny allocations**: <= 128 bytes
- **Small allocations**: > 128 bytes and <= 1024 bytes
- **Large allocations**: > 1024 bytes

Each category has its own memory zone management strategy:

- Tiny and small allocations use pre-allocated zones
- Large allocations are mapped directly from the OS

### 2. Key Data Structures

- `BlockHeader`: Metadata for memory blocks (size, real size, free status)
- `Zone`: Management structure for memory zones
- `ResultAlloc`: Result type for allocation functions

### 3. Memory Management Techniques

- Best-fit allocation strategy
- Block splitting for efficient memory use
- Memory coalescing to reduce fragmentation
- Direct mmap/munmap for large allocations

### 4. Debugging Features

- Memory leak detection
- Usage statistics
- Comprehensive test suite

## Project Structure

```
/
├── include/                # Header files
│   ├── myMalloc.h          # Public API
│   └── myMallocInternal.h  # Internal implementation details
├── src/                    # Source code
│   ├── myMalloc.c          # Core implementation
│   └── main.c              # Demo program
├── test/                   # Test suite
│   └── testMyMalloc.c      # Comprehensive tests
├── bin/                    # Compiled binaries
├── obj/                    # Object files
├── docs/                   # Documentation
└── Makefile                # Build system
```

## Recent Events

1. **Code Organization**

   - Separated code into public API and internal implementation
   - Created comprehensive test suite

2. **Build System Enhancement**

   - Added `-Werror` flag to treat warnings as errors
   - Fixed warnings to ensure clean compilation

3. **Memory Allocator Enhancements**

   - Improved memory coalescing algorithm
   - Added functional-style C code patterns
   - Enhanced error handling for edge cases

4. **Testing Improvements**
   - Added tests for edge cases
   - Implemented memory leak detection
   - Created tests for fragmentation scenarios
   - Added resource exhaustion testing

## Build and Test

The project can be built and tested using the following commands:

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
