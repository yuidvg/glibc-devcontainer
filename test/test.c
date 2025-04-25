#include "test.h"

int main()
{
    void *tiny = ftMalloc(10);
    ftFree(tiny);
    memset(tiny, 0xaa, 10);
    void *small = ftMalloc(1000);
    ftFree(small);
    memset(small, 0xbb, 1000);
    void *large = ftMalloc(1000000);
    ftFree(large);
    memset(large, 0xcc, 1000000);
    return (0);
}