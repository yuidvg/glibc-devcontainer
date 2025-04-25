#include "all.h"

const FreeChunk *findChunkInChunks(const FreeChunk *const chunkToFind, const FreeChunk *const *const chunks)
{
    const FreeChunk *current = *chunks;
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

const FreeChunk *findPreviousChunkInChunks(const FreeChunk *const chunkToFind, const FreeChunk *const *const chunks)
{
    const FreeChunk *current = *chunks;
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


void addChunkToChunks(FreeChunk *chunk, FreeChunk **const chunks)
{
    chunk->nextChunk = *chunks;
    *chunks = chunk;
}

bool removeChunkFromChunks(const FreeChunk *const chunkToRemove, const FreeChunk **const chunks)
{
    if (chunks != NULL && *chunks != NULL)
    {
        const FreeChunk *const foundChunk = findChunkInChunks(chunkToRemove, chunks);
        if (foundChunk != NULL)
        {
            const FreeChunk *const previousChunk = findPreviousChunkInChunks(chunkToRemove, chunks);
            if (previousChunk != NULL)
            {
                FreeChunk *const previousChunkMutable = (FreeChunk *const)previousChunk;
                previousChunkMutable->nextChunk = foundChunk->nextChunk;
            }
            else
            {
                *chunks = foundChunk->nextChunk;
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
