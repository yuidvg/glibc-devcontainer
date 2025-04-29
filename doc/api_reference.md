# API Reference

## Public Functions

### Memory Allocation

```c
void* myMalloc(size_t size);
```

**Description:**
Allocates memory of the specified size.

**Parameters:**

- `size`: Number of bytes to allocate

**Returns:**

- On success, a pointer to the allocated memory
- On failure, `NULL`

**Notes:**

- Memory is aligned appropriately for any built-in type
- If `size` is 0, returns `NULL`
- Allocations are categorized as tiny (≤128 bytes), small (129-1024 bytes), or large (>1024 bytes)

**Example:**

```c
// Allocate 50 bytes
void* ptr = myMalloc(50);
if (ptr) {
    // Use the memory
    memset(ptr, 0, 50);
}
```

### Memory Deallocation

```c
void myFree(void* ptr);
```

**Description:**
Frees memory previously allocated by `myMalloc`.

**Parameters:**

- `ptr`: Pointer to memory previously allocated with `myMalloc`

**Returns:**

- None

**Notes:**

- If `ptr` is NULL, no operation is performed
- Behavior is undefined if `ptr` was not allocated by `myMalloc`
- Behavior is undefined if `ptr` has already been freed
- For large allocations, memory is immediately returned to the OS
- For small/tiny allocations, memory is made available for reuse by future allocations

**Example:**

```c
void* ptr = myMalloc(100);
// Use the memory
myFree(ptr);
ptr = NULL; // Good practice to avoid dangling pointers
```

## Debugging Functions

### Print Memory Statistics

```c
void printMemoryStats(void);
```

**Description:**
Prints current memory usage statistics to stdout.

**Parameters:**

- None

**Returns:**

- None

**Output Format:**

```
Memory zones: Tiny=1, Small=1, Large=10
Total memory allocated: 1048576 bytes
```

**Example:**

```c
// Allocate some memory
void* ptr1 = myMalloc(50);
void* ptr2 = myMalloc(2000);

// Print current memory usage
printMemoryStats();
```

### Print Memory Leak Report

```c
void printMemoryLeakReport(void);
```

**Description:**
Prints a detailed report of memory leaks (if any).

**Parameters:**

- None

**Returns:**

- None

**Output Format:**

```
===== MEMORY LEAK REPORT =====
Peak memory usage: 2048576 bytes
Allocations: 50, Frees: 49, Difference: 1
MEMORY LEAKS DETECTED: 1024 bytes still allocated
  Leak 1: 0x7f8a1c003b70, size: 1024
==============================
```

**Example:**

```c
// At program termination
printMemoryLeakReport();
```

## Constants

### Memory Size Categories

```c
#define TINY_MAX_SIZE  128    // Max size for tiny allocations
#define SMALL_MAX_SIZE 1024   // Max size for small allocations
```

**Description:**
Constants defining the boundaries between allocation size categories.

**Usage:**

```c
// Allocation will use the tiny zone allocation strategy
void* tiny_ptr = myMalloc(TINY_MAX_SIZE);

// Allocation will use the small zone allocation strategy
void* small_ptr = myMalloc(SMALL_MAX_SIZE);

// Allocation will use the large allocation strategy
void* large_ptr = myMalloc(SMALL_MAX_SIZE + 1);
```

### Debug Settings

```c
#define DEBUG_MEMORY_LEAKS 1  // Whether to track allocations for leak detection
#define DEBUG_VERBOSE 0       // Whether to print verbose debug info
```

**Description:**
Constants controlling the level of debugging information.

## Usage Guidelines

1. **Initialization**: No explicit initialization is required before using `myMalloc`

2. **Memory Alignment**: All memory returned by `myMalloc` is suitably aligned for any built-in type

3. **Error Handling**: Always check if `myMalloc` returns NULL before using the allocated memory

4. **Cleanup**: Always free memory when it's no longer needed to avoid memory leaks

5. **Debugging**: Use `printMemoryStats()` and `printMemoryLeakReport()` to identify memory issues

6. **Performance**: The allocator is optimized for:
   - Fast allocation of small objects
   - Minimal fragmentation
   - Efficient reuse of freed memory
