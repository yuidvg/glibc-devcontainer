#include "all.h"

Zones zones;

__attribute__((constructor)) void initializePreAllocatedZones()
{
    // Allocate memory for the management struct itself using mmap
    const AllocResult tinyZoneResult = allocateMemory(TINY_ZONE_SIZE);
    if (tinyZoneResult.succeeded)
    {
        ChunkHeader *const tinyFreeChunksMutable = (ChunkHeader *)tinyZoneResult.allocatedMemoryAddress;
        tinyFreeChunksMutable->padSize = 0;
        tinyFreeChunksMutable->isFree = true;
        tinyFreeChunksMutable->chunkSize = TINY_ZONE_SIZE;
        tinyFreeChunksMutable->nextChunkHeader = NULL;

        zones.tinyChunks = tinyFreeChunksMutable;
        const AllocResult smallZoneResult = allocateMemory(SMALL_ZONE_SIZE);
        if (smallZoneResult.succeeded)
        {
            ChunkHeader *const smallFreeChunksMutable = (ChunkHeader *)smallZoneResult.allocatedMemoryAddress;
            smallFreeChunksMutable->chunkSize = SMALL_ZONE_SIZE;
            smallFreeChunksMutable->nextChunkHeader = NULL;
            zones.smallChunks = smallFreeChunksMutable;
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
    if (zones.tinyChunks != NULL)
    {
        unallocateMemory(zones.tinyChunks, TINY_ZONE_SIZE);
    }
    if (zones.smallChunks != NULL)
    {
        unallocateMemory(zones.smallChunks, SMALL_ZONE_SIZE);
    }
}
