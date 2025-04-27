#include "all.h"

/* The main memory allocation function */
void *ftMalloc(const size_t reqSize) // todo: pattern 0
{
    if (reqSize <= SMALL_MAX_SIZE)
    {
        if (zones.tiny.base != NULL && zones.small.base != NULL) // check if the zones are initialized
        {
            const ChunkHeader *const firstChunk = reqSize <= (size_t)TINY_MAX_SIZE
                                                      ? offsetBytes(zones.tiny.base, zones.tiny.frontPadSize)
                                                      : offsetBytes(zones.small.base, zones.small.frontPadSize);
            const size_t zoneSize = reqSize <= (size_t)TINY_MAX_SIZE ? zones.tiny.size : zones.small.size;
            const size_t needForPayload = alignUp(reqSize);
            ChunkHeader *const exact = (ChunkHeader *const)findFreeChunk(needForPayload, firstChunk, zoneSize);
            if (exact != NULL)
            {
                exact->isFree = false;
                return (chunkHeader2mem(exact));
            }
            else
            {
                ChunkHeader *const larger = findLargerFreeChunkInChunks(needForPayload, firstChunk, zoneSize);
                if (larger != NULL)
                {
                    if (larger->payloadSize >=
                        needForPayload + CHUNK_MINIMUM_SIZE) // is largerFreeChunk big enough to split?
                    {                                        // yes - split it
                        splitChunk(larger, needForPayload);
                        return (chunkHeader2mem(larger));
                    }
                    else // no - just use the largerFreeChunk
                    {
                        larger->isFree = false;
                        return (chunkHeader2mem(larger));
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
            pushLargeChunk(largeChunkHeader, &zones.large);
            return (largeChunkHeader2mem(largeChunkHeader));
        }
        else
        {
            perror("Failed to create large chunkß");
            return (NULL);
        }
    }
}
