#include "all.h"

PreallocatedZones preallocatedZones;

__attribute__((constructor)) void initializePreAllocatedZones()
{
    // Allocate memory for the management struct itself using mmap
    const AllocResult tinyZoneResult = allocateMemory(TINY_ZONE_SIZE);
    if (tinyZoneResult.succeeded)
    {
        preallocatedZones.tinyFreeChunks = (FreeChunk *)tinyZoneResult.allocatedMemoryAddress;
        preallocatedZones.tinyFreeChunks->chunkSize = TINY_ZONE_SIZE;
        preallocatedZones.tinyFreeChunks->nextChunk = NULL;
        const AllocResult smallZoneResult = allocateMemory(SMALL_ZONE_SIZE);
        if (smallZoneResult.succeeded)
        {
            preallocatedZones.smallFreeChunks = (FreeChunk *)smallZoneResult.allocatedMemoryAddress;
            preallocatedZones.smallFreeChunks->chunkSize = SMALL_ZONE_SIZE;
            preallocatedZones.smallFreeChunks->nextChunk = NULL;
        }
        else
        {
            perror("Failed to allocate memory for the small zone in constructor");
        }
    }
    else
    {
        perror("Failed to allocate memory for the management struct in constructor");
    }
}

__attribute__((destructor)) void cleanupPreAllocatedZones()
{
    if (preallocatedZones.tinyFreeChunks != NULL)
    {
        unallocateMemory(preallocatedZones.tinyFreeChunks, TINY_ZONE_SIZE);
    }
    if (preallocatedZones.smallFreeChunks != NULL)
    {
        unallocateMemory(preallocatedZones.smallFreeChunks, SMALL_ZONE_SIZE);
    }
}

/* The main memory allocation function */
void *ftMalloc(const size_t size)
{
    (void)size;
    return (NULL);
}
