#include "all.h"

static bool unallocateLargeChunk(const LargeChunkHeader *const victim)
{
    if (victim != NULL)
    {
        pop(victim);
        if (unallocateMemory(toBase(victim), toBaseSize(victim)))
            return true;
    }
    return false;
}

/* Free memory previously allocated with myMalloc */
void ftFree(const void *const mem)
{
    if (mem != NULL)
    {
        const LargeChunkHeader *const largeChunk = findLargeChunk(mem);
        if (largeChunk != NULL) // large chunk
        {
            unallocateLargeChunk(largeChunk);
        }
        else if (findChunk(mem, &zones.tiny) != NULL) // small chunk
        {
            ChunkHeader *const chunk = findChunk(mem, &zones.tiny);
            chunk->isFree = true;
        }
        else if (findChunk(mem, &zones.small) != NULL) // small chunk
        {
            ChunkHeader *const chunk = findChunk(mem, &zones.small);
            chunk->isFree = true;
        }
        else
        {
            perror("Invalid memory address");
        }
    }
}
