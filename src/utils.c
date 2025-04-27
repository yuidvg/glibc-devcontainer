#include "all.h"

ChunkHeader *next(const ChunkHeader *const original)
{
    return (ChunkHeader *)offsetBytes(original, original->payloadSize);
}

const ChunkHeader *findFreeChunk(const size_t payloadSize, const ChunkHeader *const chunks, const size_t zoneSize)
{
    const ChunkHeader *current = chunks;
    size_t seenSize = 0;
    while (seenSize < zoneSize)
    {
        if (current->payloadSize >= payloadSize && current->isFree)
        {
            return current;
        }
        seenSize += current->payloadSize + CHUNK_HEADER_SIZE;
        current = next(current);
    }
    return NULL;
}

const ChunkHeader *findLargerFreeChunkInChunks(const size_t payloadSize, const ChunkHeader *const chunks,
                                               const size_t zoneSize)
{
    const ChunkHeader *current = chunks;
    size_t seenSize = 0;
    while (seenSize < zoneSize)
    {
        if (current->payloadSize >= payloadSize && current->isFree)
        {
            return current;
        }
        seenSize += current->payloadSize + CHUNK_HEADER_SIZE;
        current = next(current);
    }
    return NULL;
}

void splitChunk(ChunkHeader *const main, const size_t goal)
{
    const size_t restSize = CHUNK_HEADER_SIZE + main->payloadSize - goal;
    ChunkHeader *const rest = (ChunkHeader *const)offsetBytes(main, goal);
    rest->payloadSize = restSize;
    rest->isFree = true;
    main->payloadSize = goal;
}

void pushLargeChunk(LargeChunkHeader *newbie, LargeChunkHeader **const group)
{
    newbie->next = *group;
    *group = newbie;
}

bool popLargeChunk(const LargeChunkHeader *const victim, LargeChunkHeader **const group)
{
    if (group != NULL && *group != NULL)
    {
        const LargeChunkHeader *const found = findLargeChunk(victim, *group);
        if (found != NULL)
        {
            LargeChunkHeader *const previous = findPreviousLargeChunk(victim, *group);
            if (previous != NULL)
            {
                previous->next = found->next;
            }
            else
            {
                *group = found->next;
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

LargeChunkHeader *createLargeChunk(const size_t reqSize)
{
    const size_t allocationSize = reqsize2AllocationSize(reqSize);
    const AllocResult allocResult = allocateMemory(allocationSize);
    if (allocResult.succeeded)
    {
        const size_t frontPadSize = distanceToNextAlignment(allocResult.allocatedMemoryAddress);
        LargeChunkHeader *const newbie =
            (LargeChunkHeader *const)offsetBytes(allocResult.allocatedMemoryAddress, frontPadSize);
        newbie->frontPadSize = frontPadSize;
        newbie->payloadSize = allocationSize - frontPadSize;
        newbie->next = NULL;
        return newbie;
    }
    else
    {
        perror("Failed to allocate memory for ftMalloc");
        return (NULL);
    }
}
