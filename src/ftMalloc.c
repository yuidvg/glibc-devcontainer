#include "all.h"

static bool defrag()
{
    if (zones.tiny.base != NULL && zones.small.base != NULL)
    {
        
        return true;
    }
    else
    {
        return false;
    }
}

/* The main memory allocation function */
void *ftMalloc(const size_t reqSize) // todo: pattern 0
{
    if (reqSize <= SMALL_MAX_SIZE)
    {
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
                        needForPayload + CHUNK_MINIMUM_SIZE) // is largerFreeChunk big enough to split?
                    {                                        // yes - split it
                        splitChunk(fittable, needForPayload);
                        fittable->isFree = false;
                        return (chunkHeader2mem(fittable));
                    }
                    else // no - just use the largerFreeChunk
                    {
                        fittable->isFree = false;
                        return (chunkHeader2mem(fittable));
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
        LargeChunkHeader *const largeChunkHeader = createLargeChunk(reqSize);
        if (largeChunkHeader != NULL)
        {
            push(largeChunkHeader);
            return (largeChunk2Mem(largeChunkHeader));
        }
        else
        {
            perror("Failed to create large chunk");
            return (NULL);
        }
    }
}
