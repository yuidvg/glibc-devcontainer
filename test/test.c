#include "test.h"

int main()
{
    void *tiny = ftMalloc(10);
    memset(tiny, 0xaa, 10);
    void *small = ftMalloc(1000);
    memset(small, 0xbb, 1000);
    void *large = ftMalloc(1000000);
    memset(large, 0xcc, 1000000);
    show_alloc_mem();
    ftFree(small);
    ftFree(tiny);
    ftFree(large);
    show_alloc_mem();
    return (0);
}