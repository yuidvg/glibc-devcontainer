#include "all.h"

const size_t memsize(const ChunkHeader *const chunkHeader)
{
    return chunkHeader->chunkSize - chunkHeader->padSize - CHUNK_HEADER_SIZE;
}

const ChunkHeader *findChunkInChunks(const ChunkHeader *const chunkToFind, const ChunkHeader *const chunks)
{
    const ChunkHeader *current = chunks;
    while (current != NULL)
    {
        if (current == chunkToFind)
        {
            return current;
        }
        current = current->nextChunkHeader;
    }
    return NULL;
}

const ChunkHeader *findPreviousChunkInChunks(const ChunkHeader *const chunkToFind, const ChunkHeader *const chunks)
{
    const ChunkHeader *current = chunks;
    while (current != NULL)
    {
        if (current->nextChunkHeader == chunkToFind)
        {
            return current;
        }
        current = current->nextChunkHeader;
    }
    return NULL;
}

const ChunkHeader *findChunkBySizeInChunks(const size_t chunkSizeToFind, const ChunkHeader *const chunks)
{
    const ChunkHeader *current = chunks;
    while (current != NULL)
    {
        if (current->chunkSize == chunkSizeToFind)
        {
            return current;
        }
        current = current->nextChunkHeader;
    }
    return NULL;
}

const ChunkHeader *findLargerChunkInChunks(const size_t standardChunkSize, const ChunkHeader *const chunks)
{
    const ChunkHeader *current = chunks;
    while (current != NULL)
    {
        if (current->chunkSize > standardChunkSize)
        {
            return current;
        }
        current = current->nextChunkHeader;
    }
    return NULL;
}

void addChunkToChunks(ChunkHeader *chunk, ChunkHeader **const headToChunks)
{
    chunk->nextChunkHeader = *headToChunks;
    *headToChunks = chunk;
}

bool removeChunkFromChunks(const ChunkHeader *const chunkToRemove, const ChunkHeader **const headToChunks)
{
    if (headToChunks != NULL && *headToChunks != NULL)
    {
        const ChunkHeader *const foundChunk = findChunkInChunks(chunkToRemove, *headToChunks);
        if (foundChunk != NULL)
        {
            const ChunkHeader *const previousChunk = findPreviousChunkInChunks(chunkToRemove, *headToChunks);
            if (previousChunk != NULL)
            {
                ChunkHeader *const previousChunkMutable = (ChunkHeader *const)previousChunk;
                previousChunkMutable->nextChunkHeader = foundChunk->nextChunkHeader;
            }
            else
            {
                *headToChunks = foundChunk->nextChunkHeader;
            }
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

ChunkHeader *createChunk(const size_t reqSize)
{
    const size_t chunksize = reqsize2chunksize(reqSize);
    const AllocResult allocResult = allocateMemory(chunksize);
    if (allocResult.succeeded)
    {
        const size_t padSize = rawAllocatedMemory2padSize(allocResult.allocatedMemoryAddress);
        ChunkHeader *const chunkHeaderPointer =
            (ChunkHeader *const)offsetBytes(allocResult.allocatedMemoryAddress, padSize);
        chunkHeaderPointer->chunkSize = chunksize;
        chunkHeaderPointer->padSize = padSize;
        chunkHeaderPointer->isFree = true;
        chunkHeaderPointer->nextChunkHeader = NULL;
        return (chunkHeader2mem(chunkHeaderPointer));
    }
    else
    {
        perror("Failed to allocate memory for ftMalloc");
        return (NULL);
    }
}

SplitChunks splitChunk(const ChunkHeader *const chunkToSplit, const size_t sizeToCutout)
{
    const size_t restSize = chunkToSplit->chunkSize - sizeToCutout;
    ChunkHeader *const restMutable = (ChunkHeader *const)offsetBytes(chunkToSplit, sizeToCutout);
    restMutable->chunkSize = restSize;
    restMutable->nextChunkHeader = chunkToSplit->nextChunkHeader;
    ChunkHeader *const mainMutable = (ChunkHeader *const)chunkToSplit;
    mainMutable->chunkSize = sizeToCutout;
    mainMutable->nextChunkHeader = restMutable;
    return (SplitChunks){chunkToSplit, restMutable};
}
