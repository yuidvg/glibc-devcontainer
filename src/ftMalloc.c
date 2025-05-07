#include "all.h"

static bool defragSection(ChunkHeader *const start, const void *const end)
{
    ChunkHeader *const first = findFree(toNext(start), end);
    if (first != NULL)
    {
        expandChunk(first, CHUNK_MINIMUM_SIZE);
        defragSection(toNext(first), end);
        return true;
    }
    return false;
}

__attribute__((unused)) static bool defrag()
{
    if (zones.tiny.base != NULL && zones.small.base != NULL)
    {
        defragSection(toFirstChunk(&zones.tiny), toZoneEnd(&zones.tiny));
        defragSection(toFirstChunk(&zones.small), toZoneEnd(&zones.small));
        return true;
    }
    return false;
}

/* The main memory allocation function */
void *ftMalloc(const size_t reqSize) // todo: pattern 0
{
    if (reqSize <= SMALL_MAX_SIZE)
    {
        defrag();
        if (zones.tiny.base != NULL && zones.small.base != NULL) // check if the zones are initialized
        {
            const size_t needForPayload = alignUp(reqSize);
            const Zone *const zone = reqSize <= (size_t)TINY_MAX_SIZE ? &zones.tiny : &zones.small;
            ChunkHeader *const exact = (ChunkHeader *const)findFreeChunk(needForPayload, zone);
            if (exact != NULL)
            {
                exact->isFree = false;
                return (chunkHeader2mem(exact));
            }
            else
            {
                ChunkHeader *const fittable = findFittableFreeChunk(needForPayload, zone);
                if (fittable != NULL)
                {
                    if (fittable->payloadSize >=
                        needForPayload + CHUNK_MINIMUM_SIZE) // is fittable chunk big enough to split?
                    {                                        // yes - split it
                        splitChunk(fittable, needForPayload);
                        fittable->isFree = false;
                        return (chunkHeader2mem(fittable));
                    }
                    else // no - just use the fittable chunk
                    {
                        fittable->isFree = false;
                        return (chunkHeader2mem(fittable));
                    }
                }
                else
                {
                    LargeChunkHeader *const largeChunkHeader = createLargeChunk(reqSize);
                    if (largeChunkHeader != NULL)
                    {
                        push(largeChunkHeader);
                        return (largeChunk2Mem(largeChunkHeader));
                    }
                    else
                    {
                        printError("Failed to create large chunk(but for tiny/small zones fallback)");
                        return (NULL);
                    }
                }
            }
        }
        else
        {
            printError("Preallocated zones are not initialized correctly");
            return (NULL);
        }
    }
    else
    {
        LargeChunkHeader *const largeChunkHeader = createLargeChunk(reqSize);
        if (largeChunkHeader != NULL)
        {
            push(largeChunkHeader);
            return (largeChunk2Mem(largeChunkHeader));
        }
        else
        {
            printError("Failed to create large chunk");
            return (NULL);
        }
    }
}
