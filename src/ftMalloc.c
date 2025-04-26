#include "all.h"

/* The main memory allocation function */
void *ftMalloc(const size_t reqSize)
{
    const size_t chunkSize = reqSize + CHUNK_HEADER_SIZE;
    if (reqSize <= SMALL_MAX_SIZE)
    {
        if (zones.smallChunks != NULL && zones.tinyChunks != NULL)
        {
            const ChunkHeader *const *const freeChunksToSearchPointer = reqSize <= (size_t)TINY_MAX_SIZE ? &zones.tinyChunks : &zones.smallChunks;
            const ChunkHeader *const freeChunk = findChunkBySizeInChunks(chunkSize, *freeChunksToSearchPointer);
            if (freeChunk != NULL)
            {
                removeChunkFromChunks(freeChunk, (const ChunkHeader **const)freeChunksToSearchPointer);
                return ((void *)(freeChunk + CHUNK_HEADER_SIZE));
            }
            else
            {
                const ChunkHeader *const largerFreeChunk = findLargerChunkInChunks(chunkSize, *freeChunksToSearchPointer);
                if (largerFreeChunk != NULL)
                {
                    if (largerFreeChunk->chunkSize > chunkSize + CHUNK_HEADER_SIZE) // is largerFreeChunk big enough to split?
                    { // yes - split it
                        const SplitChunks splitChunks = splitChunk(largerFreeChunk, chunkSize);
                        removeChunkFromChunks(splitChunks.main, (const ChunkHeader **const)freeChunksToSearchPointer);
                        return ((void *)splitChunks.main + CHUNK_HEADER_SIZE);
                    }
                    else // no - just use the largerFreeChunk
                    {
                        removeChunkFromChunks(largerFreeChunk, (const ChunkHeader **const)freeChunksToSearchPointer);
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
