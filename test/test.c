#include "test.h"

int main()
{
    void *tiny = ftMalloc(10);
    memset(tiny, 0xaa, 10);
    void *tiny2 = ftMalloc(20);
    memset(tiny2, 0xbb, 20);
    void *small = ftMalloc(1000);
    memset(small, 0xcc, 1000);
    void *small2 = ftMalloc(1000);
    memset(small2, 0xdd, 1000);
    void *large = ftMalloc(1000000);
    memset(large, 0xee, 1000000);
    void *large2 = ftMalloc(1000000);
    memset(large2, 0xff, 1000000);
    show_alloc_mem();
    ftFree(small);
    ftFree(small2);
    ftFree(tiny);
    show_alloc_mem();
    ftFree(tiny2);
    ftFree(large);
    ftFree(large2);
    return (0);
}
