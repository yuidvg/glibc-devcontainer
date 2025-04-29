#pragma once

/**
 * @file myMalloc.h
 * @brief Memory allocator implementation with pre-allocated zones.
 *
 * This memory allocator uses mmap/munmap for memory management and organizes
 * allocations into three categories: TINY, SMALL, and LARGE, with different
 * handling strategies for each.
 */

#include <stdlib.h>

/* Function prototypes - implementation functions */
void free(void *ptr);
void *malloc(size_t size);
void *realloc(void *ptr, size_t size);

void show_alloc_mem();

