#include "all.h"

Global global;

__attribute__((constructor)) void initializePreAllocatedZones()
{
    // Allocate memory for the management struct itself using mmap
    const AllocResult tinyZoneResult = allocateMemory(TINY_ZONE_SIZE);
    if (tinyZoneResult.succeeded)
    {
        Chunk *const tinyFreeChunksMutable = (Chunk *)tinyZoneResult.allocatedMemoryAddress;
        tinyFreeChunksMutable->chunkSize = TINY_ZONE_SIZE;
        tinyFreeChunksMutable->nextChunk = NULL;

        global.tinyChunks = tinyFreeChunksMutable;
        const AllocResult smallZoneResult = allocateMemory(SMALL_ZONE_SIZE);
        if (smallZoneResult.succeeded)
        {
            Chunk *const smallFreeChunksMutable = (Chunk *)smallZoneResult.allocatedMemoryAddress;
            smallFreeChunksMutable->chunkSize = SMALL_ZONE_SIZE;
            smallFreeChunksMutable->nextChunk = NULL;
            global.smallChunks = smallFreeChunksMutable;
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
    if (global.tinyChunks != NULL)
    {
        unallocateMemory(global.tinyChunks, TINY_ZONE_SIZE);
    }
    if (global.smallChunks != NULL)
    {
        unallocateMemory(global.smallChunks, SMALL_ZONE_SIZE);
    }
}

/* The main memory allocation function */
void *ftMalloc(const size_t memSize)
{
    const size_t chunkSize = memSize + CHUNK_HEADER_SIZE;
    if (memSize <= SMALL_MAX_SIZE)
    {
        if (global.smallChunks != NULL && global.tinyChunks != NULL)
        {
            const Chunk *const *const freeChunksToSearchPointer = memSize <= (size_t)TINY_MAX_SIZE ? &global.tinyChunks : &global.smallChunks;
            const Chunk *const freeChunk = findChunkBySizeInChunks(chunkSize, *freeChunksToSearchPointer);
            if (freeChunk != NULL)
            {
                removeChunkFromChunks(freeChunk, (const Chunk **const)freeChunksToSearchPointer);
                return ((void *)(freeChunk + CHUNK_HEADER_SIZE));
            }
            else
            {
                const Chunk *const largerFreeChunk = findLargerChunkInChunks(chunkSize, *freeChunksToSearchPointer);
                if (largerFreeChunk != NULL)
                {
                    if (largerFreeChunk->chunkSize > chunkSize + CHUNK_HEADER_SIZE) // is largerFreeChunk big enough to split?
                    { // yes - split it
                        const SplitChunks splitChunks = splitChunk(largerFreeChunk, chunkSize);
                        removeChunkFromChunks(splitChunks.main, (const Chunk **const)freeChunksToSearchPointer);
                        return ((void *)splitChunks.main + CHUNK_HEADER_SIZE);
                    }
                    else // no - just use the largerFreeChunk
                    {
                        removeChunkFromChunks(largerFreeChunk, (const Chunk **const)freeChunksToSearchPointer);
                        return ((void *)(largerFreeChunk + CHUNK_HEADER_SIZE));
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
