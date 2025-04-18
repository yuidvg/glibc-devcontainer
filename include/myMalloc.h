#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stdlib.h>
#include <stdbool.h>

/**
 * @file myMalloc.h
 * @brief Memory allocator implementation with pre-allocated zones.
 *
 * This memory allocator uses mmap/munmap for memory management and organizes
 * allocations into three categories: TINY, SMALL, and LARGE, with different
 * handling strategies for each.
 */

/* Size constants */
#define TINY_MAX_SIZE 128	/* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024 /* m: max size for small allocations */

/* Function prototypes - implementation functions */
void *my_malloc(size_t size);
void my_free(void *ptr);
void print_memory_stats(void);
void print_memory_leak_report(void);

/* System override functions */
void *malloc(size_t size);
void free(void *ptr);

#endif /* MY_MALLOC_H */