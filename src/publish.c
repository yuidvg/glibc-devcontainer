#include "all.h"

void *malloc(size_t size)
{
    return ftMalloc(size);
}

void free(void *ptr)
{
    ftFree(ptr);
}


void *realloc(void *ptr, size_t size)
{
    return ftRealloc(ptr, size);
}

void show_alloc_mem()
{
    show_allocated_memories();
}