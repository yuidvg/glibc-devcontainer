#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include "my_malloc.h"

void *my_malloc(size_t size)
{
    // Simple placeholder implementation
    printf("my_malloc called with size: %zu\n", size);
    return malloc(size);  // Using system malloc for now
}

void my_free(void *ptr)
{
    // Simple placeholder implementation
    printf("my_free called with pointer: %p\n", ptr);
    free(ptr);  // Using system free for now
}

void print_memory_stats(void)
{
    // Simple placeholder implementation
    printf("Memory stats: Not implemented yet\n");
}

void print_memory_leak_report(void)
{
    // Simple placeholder implementation
    printf("Memory leak report: Not implemented yet\n");
}