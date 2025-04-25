#include "all.h"

/* Free memory previously allocated with myMalloc */
void ftFree(const void *const mem)
{
    if (mem != NULL)
    {
        const void *const chunkToFree = mem - sizeof(size_t);
        const size_t chunkSize = *(size_t *)(chunkToFree);
        const size_t memSize = chunkSize - sizeof(size_t);
        if (memSize <= SMALL_MAX_SIZE) // tiny/small chunk
        {
            const Chunk *const freeChunk = (Chunk *)(chunkToFree);
            addChunkToChunks((Chunk *const)freeChunk,
                             (Chunk **const)(memSize <= TINY_MAX_SIZE ? &preallocatedZones.tinyFreeChunks
                                                                                : &preallocatedZones.smallFreeChunks));
            printf("tiny/small chunk freed successfully\n");
        }
        else // large chunk
        {
            if (unallocateMemory(chunkToFree, chunkSize))
            {
                printf("large chunk freed successfully\n");
            }
            else
            {
                printf("large chunk freeing failed\n");
            }
        }
    }
}
