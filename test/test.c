#include "test.h"

int main()
{
    void *tiny = ftMalloc(10);
    memset(tiny, 0xaa, 10);
    ftFree(tiny);
    void *small = ftMalloc(1000);
    memset(small, 0xbb, 1000);
    ftFree(small);
    void *large = ftMalloc(1000000);
    memset(large, 0xcc, 1000000);
    ftFree(large);
    return (0);
}