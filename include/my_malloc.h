#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file my_malloc.h
 * @brief Memory allocator implementation with pre-allocated zones.
 *
 * This memory allocator uses mmap/munmap for memory management and organizes
 * allocations into three categories: TINY, SMALL, and LARGE, with different
 * handling strategies for each.
 */

/* Constants for memory allocation categories */
#define TINY_MAX_SIZE 128	/* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024 /* m: max size for small allocations */

/**
 * @brief Allocate memory of the given size.
 *
 * Allocates memory of the specified size. The memory is not initialized.
 *
 * @param size The size in bytes to allocate
 * @return void* Pointer to the allocated memory, or NULL if allocation fails
 */
void *my_malloc(size_t size);

/**
 * @brief Free previously allocated memory.
 *
 * Frees memory that was previously allocated by my_malloc.
 * If ptr is NULL, no operation is performed.
 *
 * @param ptr Pointer to the memory to free
 */
void my_free(void *ptr);

/**
 * @brief Function to print memory allocation statistics.
 *
 * @note This is a debugging function and may be removed in production.
 */
void print_memory_stats(void);

/**
 * @brief Function to print memory leak report.
 *
 * @note This is a debugging function and may be removed in production.
 */
void print_memory_leak_report(void);

#endif /* MY_MALLOC_H */