#include "all.h"

void *ftRealloc(const void *const originalMem, const size_t reqSize)
{
    if (originalMem == NULL)
    {
        return ftMalloc(reqSize);
    }

    if (reqSize == 0)
    {
        ftFree(originalMem);
        return NULL;
    }

    const LargeChunkHeader *const largeChunk = findLargeChunk(originalMem);
    if (largeChunk != NULL) // large chunk
    {
        void *const new = ftMalloc(reqSize);
        if (new != NULL)
        {
            ft_memcpy(new, originalMem, min(largeChunk->payloadSize, reqSize));
            ftFree(originalMem);
            return new;
        }
        else
        {
            return NULL;
        }
    }
    else // tiny/small chunk ?
    {
        const Zone *const zone = toZone(originalMem);
        if (zone != NULL)
        {
            const size_t needForPayload = alignUp(reqSize);
            ChunkHeader *const originalChunk = findChunk(originalMem, zone);
            size_t consequtiveFreeSize = consequtiveFreeChunksSize(toNext(originalChunk));
            if (originalChunk->payloadSize + consequtiveFreeSize >= needForPayload) // can be expanded in-place
            { // yes - expand in-place -> split to optimize
                expandChunk(originalChunk, consequtiveFreeSize);
                splitChunk(originalChunk, needForPayload);
                return (void *)originalChunk;
            }
            else
            { // no - allocate new memory -> copy -> free old memory
                void *const new = ftMalloc(reqSize);
                if (new != NULL)
                {
                    ft_memcpy(new, originalMem, min(originalChunk->payloadSize, reqSize));
                    ftFree(originalMem);
                    return new;
                }
                else
                {
                    return NULL;
                }
            }
        }
        else
        {
            return NULL;
        }
    }
}
