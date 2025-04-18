#pragma once

#include <stdbool.h>
#include <stdlib.h>

/* Size constants */
#define TINY_MAX_SIZE 128   /* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024 /* m: max size for small allocations */

/* Function prototypes */
void *my_malloc(size_t size);
void my_free(void *ptr);
void print_memory_stats(void);
void print_memory_leak_report(void);
