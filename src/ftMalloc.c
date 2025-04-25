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
void *ftMalloc(const size_t memSize)
{
    const size_t chunkSize = memSize + CHUNK_HEADER_SIZE;
    if (memSize <= SMALL_MAX_SIZE)
    {
        if (preallocatedZones.smallFreeChunks != NULL && preallocatedZones.tinyFreeChunks != NULL &&
            preallocatedZones.tinyFreeChunks->chunkSize == TINY_ZONE_SIZE &&
            preallocatedZones.smallFreeChunks->chunkSize == SMALL_ZONE_SIZE)
        {
            const FreeChunk *const freeChunksToSearch = memSize <= TINY_MAX_SIZE ? &preallocatedZones.tinyFreeChunks : &preallocatedZones.smallFreeChunks;
            const FreeChunk *const freeChunk = findChunkBySizeInChunks(chunkSize, freeChunksToSearch);
            if (freeChunk != NULL)
            {
                removeChunkFromChunks(freeChunk, &freeChunksToSearch);
                return (freeChunk + CHUNK_HEADER_SIZE);
            }
            else
            {
                const FreeChunk *const largerFreeChunk = findLargerChunkInChunks(chunkSize, freeChunksToSearch);
                if (largerFreeChunk != NULL)
                {
                    if (largerFreeChunk->chunkSize > chunkSize + CHUNK_HEADER_SIZE) // is largerFreeChunk big enough to split?
                    { // yes - split it
                        const SplitChunks splitChunks = splitChunk(largerFreeChunk, chunkSize);
                        removeChunkFromChunks(splitChunks.main, &freeChunksToSearch);
                        return (splitChunks.main + CHUNK_HEADER_SIZE);
                    }
                    else // no - just use the largerFreeChunk
                    {
                        removeChunkFromChunks(largerFreeChunk, &freeChunksToSearch);
                        return (largerFreeChunk + CHUNK_HEADER_SIZE);
                    }
                }
                else
                {
                    perror("No free chunk found");
                    return (NULL);
                }
            }
        }
        else
        {
            perror("Preallocated zones are not initialized correctly");
            return (NULL);
        }
    }
    else
    {
        const AllocResult allocResult = allocateMemory(chunkSize);
        if (allocResult.succeeded)
        {
            size_t *chunkSizeMutablePointer = (size_t *)allocResult.allocatedMemoryAddress;
            *chunkSizeMutablePointer = chunkSize;
            return (allocResult.allocatedMemoryAddress + CHUNK_HEADER_SIZE);
        }
        else
        {
            perror("Failed to allocate memory for ftMalloc");
            return (NULL);
        }
    }
}
