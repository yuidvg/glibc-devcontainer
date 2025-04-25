#include "all.h"

const FreeChunk *findChunkInChunks(const FreeChunk *const chunkToFind, const FreeChunk *const chunks)
{
    const FreeChunk *current = chunks;
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

const FreeChunk *findPreviousChunkInChunks(const FreeChunk *const chunkToFind, const FreeChunk *const chunks)
{
    const FreeChunk *current = chunks;
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

const FreeChunk *findChunkBySizeInChunks(const size_t chunkSizeToFind, const FreeChunk *const chunks)
{
    const FreeChunk *current = chunks;
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

const FreeChunk *findLargerChunkInChunks(const size_t standardChunkSize, const FreeChunk *const chunks)
{
    const FreeChunk *current = chunks;
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

void addChunkToChunks(FreeChunk *chunk, FreeChunk **const headToChunks)
{
    chunk->nextChunk = *headToChunks;
    *headToChunks = chunk;
}

bool removeChunkFromChunks(const FreeChunk *const chunkToRemove, const FreeChunk **const headToChunks)
{
    if (headToChunks != NULL && *headToChunks != NULL)
    {
        const FreeChunk *const foundChunk = findChunkInChunks(chunkToRemove, *headToChunks);
        if (foundChunk != NULL)
        {
            const FreeChunk *const previousChunk = findPreviousChunkInChunks(chunkToRemove, *headToChunks);
            if (previousChunk != NULL)
            {
                FreeChunk *const previousChunkMutable = (FreeChunk *const)previousChunk;
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

SplitChunks splitChunk(const FreeChunk *const chunkToSplit, const size_t sizeToCutout)
{
    const size_t restSize = chunkToSplit->chunkSize - sizeToCutout;
    FreeChunk *const restMutable = (FreeChunk *const)offsetBytes(chunkToSplit, sizeToCutout);
    restMutable->chunkSize = restSize;
    restMutable->nextChunk = chunkToSplit->nextChunk;
    FreeChunk *const mainMutable = (FreeChunk *const)chunkToSplit;
    mainMutable->chunkSize = sizeToCutout;
    mainMutable->nextChunk = restMutable;
    return (SplitChunks){chunkToSplit, restMutable};
}
