#include "all.h"

const Chunk *findChunkInChunks(const Chunk *const chunkToFind, const Chunk *const chunks)
{
    const Chunk *current = chunks;
    while (current != NULL)
    {
        if (current == chunkToFind)
        {
            return current;
        }
        current = current->nextChunk;
    }
    return NULL;
}

const Chunk *findPreviousChunkInChunks(const Chunk *const chunkToFind, const Chunk *const chunks)
{
    const Chunk *current = chunks;
    while (current != NULL)
    {
        if (current->nextChunk == chunkToFind)
        {
            return current;
        }
        current = current->nextChunk;
    }
    return NULL;
}

const Chunk *findChunkBySizeInChunks(const size_t chunkSizeToFind, const Chunk *const chunks)
{
    const Chunk *current = chunks;
    while (current != NULL)
    {
        if (current->chunkSize == chunkSizeToFind)
        {
            return current;
        }
        current = current->nextChunk;
    }
    return NULL;
}

const Chunk *findLargerChunkInChunks(const size_t standardChunkSize, const Chunk *const chunks)
{
    const Chunk *current = chunks;
    while (current != NULL)
    {
        if (current->chunkSize > standardChunkSize)
        {
            return current;
        }
        current = current->nextChunk;
    }
    return NULL;
}

void addChunkToChunks(Chunk *chunk, Chunk **const headToChunks)
{
    chunk->nextChunk = *headToChunks;
    *headToChunks = chunk;
}

bool removeChunkFromChunks(const Chunk *const chunkToRemove, const Chunk **const headToChunks)
{
    if (headToChunks != NULL && *headToChunks != NULL)
    {
        const Chunk *const foundChunk = findChunkInChunks(chunkToRemove, *headToChunks);
        if (foundChunk != NULL)
        {
            const Chunk *const previousChunk = findPreviousChunkInChunks(chunkToRemove, *headToChunks);
            if (previousChunk != NULL)
            {
                Chunk *const previousChunkMutable = (Chunk *const)previousChunk;
                previousChunkMutable->nextChunk = foundChunk->nextChunk;
            }
            else
            {
                *headToChunks = foundChunk->nextChunk;
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

void *offsetBytes(const void *const pointer, const size_t offset)
{
    const uint8_t *const baseBytes = (const uint8_t *const)pointer;

    // one mutable local for the eventual return
    void *resultPointer = (void *)(baseBytes + offset);

    return resultPointer;
}

SplitChunks splitChunk(const Chunk *const chunkToSplit, const size_t sizeToCutout)
{
    const size_t restSize = chunkToSplit->chunkSize - sizeToCutout;
    Chunk *const restMutable = (Chunk *const)offsetBytes(chunkToSplit, sizeToCutout);
    restMutable->chunkSize = restSize;
    restMutable->nextChunk = chunkToSplit->nextChunk;
    Chunk *const mainMutable = (Chunk *const)chunkToSplit;
    mainMutable->chunkSize = sizeToCutout;
    mainMutable->nextChunk = restMutable;
    return (SplitChunks){chunkToSplit, restMutable};
}
