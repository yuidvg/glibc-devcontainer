#include "all.h"

AllocResult allocateMemory(const size_t size)
{
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
    {
        return (AllocResult){.succeeded = false, .allocatedMemoryAddress = NULL};
    }
    return (AllocResult){.succeeded = true, .allocatedMemoryAddress = ptr};
}

bool unallocateMemory(void *const ptr, const size_t size)
{
    if (munmap(ptr, size) == -1)
    {
        return false;
    }
    return true;
}
