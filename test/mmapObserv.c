#define _DEFAULT_SOURCE
#include "test.h"
#include <sys/mman.h>

int main(int argc, char *argv[])
{
    const long parsed_long = strtol(argv[1], NULL, 10); // Base 10

    const size_t size_arg = (size_t)parsed_long;

    void *ptr = mmap(NULL, size_arg, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr != MAP_FAILED)
    {
        printf("mmap succeeded\n");
        munmap(ptr, size_arg);
        printf("munmap succeeded\n");
        return 0;
    }
    else
    {
        perror("mmap");
        return 1;
    }
}
