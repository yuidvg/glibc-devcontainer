#include "all.h"

// zone
size_t toChunkAreaSize(const Zone *const zone)
{
    return zone->baseSize - zone->frontPadSize;
}

ChunkHeader *toFirstChunk(const Zone *const zone)
{
    return (ChunkHeader *)offsetBytes(zone->base, zone->frontPadSize);
}

// chunk

Zone *toZone(const void *const mem)
{
    const ChunkHeader *const foundTiny = findChunk(mem, &zones.tiny);
    if (foundTiny != NULL)
    {
        return &zones.tiny;
    }
    else
    {
        const ChunkHeader *const foundSmall = findChunk(mem, &zones.small);
        if (foundSmall != NULL)
        {
            return &zones.small;
        }
        else
        {
            return NULL;
        }
    }
}

void *toZoneEnd(const Zone *const zone)
{
    return offsetBytes(zone->base, zone->baseSize);
}

void *chunk2Mem(const ChunkHeader *const chunk)
{
    return (void *)offsetBytes(chunk, CHUNK_HEADER_SIZE);
}

ChunkHeader *toNext(const ChunkHeader *const original)
{
    return (ChunkHeader *)offsetBytes(original, toChunkSize(original));
}

size_t toChunkSize(const ChunkHeader *const chunk)
{
    return chunk->payloadSize + CHUNK_HEADER_SIZE;
}

ChunkHeader *findChunk(const void *const mem, const Zone *const zone)
{
    ChunkHeader *current = toFirstChunk(zone);
    size_t seenSize = 0;
    while (seenSize < toChunkAreaSize(zone))
    {
        if (chunk2Mem(current) == mem)
        {
            return current;
        }
        seenSize += toChunkSize(current);
        current = toNext(current);
    }
    return NULL;
}

const ChunkHeader *findFreeChunk(const size_t payloadSize, const Zone *const zone)
{
    ChunkHeader *current = toFirstChunk(zone);
    size_t seenSize = 0;
    while (seenSize < toChunkAreaSize(zone))
    {
        if (current->payloadSize == payloadSize && current->isFree)
        {
            return current;
        }
        seenSize += toChunkSize(current);
        current = toNext(current);
    }
    return NULL;
}

ChunkHeader *findFittableFreeChunk(const size_t payloadSize, const Zone *const zone)
{
    ChunkHeader *current = toFirstChunk(zone);
    size_t seenSize = 0;
    while (seenSize < toChunkAreaSize(zone))
    {
        if (current->payloadSize >= payloadSize && current->isFree)
        {
            return current;
        }
        seenSize += toChunkSize(current);
        current = toNext(current);
    }
    return NULL;
}

void splitChunk(ChunkHeader *const main, const size_t goalPayload)
{
    ChunkHeader *const rest = (ChunkHeader *const)offsetBytes(main, CHUNK_HEADER_SIZE + goalPayload);
    rest->payloadSize = main->payloadSize - goalPayload - CHUNK_HEADER_SIZE;
    rest->isFree = true;
    main->payloadSize = goalPayload;
}

size_t consequtiveFreeChunksSize(const ChunkHeader *const start)
{
    const Zone *const zone = toZone(start);
    if (zone != NULL)
    {
        const ChunkHeader *const zoneEnd = toZoneEnd(zone);
        size_t size = 0;
        const ChunkHeader *current = start;
        while (current < zoneEnd && current->isFree)
        {
            size += toChunkSize(current);
            current = toNext(current);
        }
        return size;
    }
    else
    {
        return 0;
    }
}

bool expandChunk(ChunkHeader *const expandee, const size_t by)
{
    if (isAligned(by) && by > alignUp(1))
    {
        expandee->payloadSize += by;
    }
    return false;
}

// largeChunk

void *largeChunk2Mem(const LargeChunkHeader *const header)
{
    return offsetBytes(header, LARGE_CHUNK_HEADER_SIZE);
}

void *toBase(const LargeChunkHeader *const header)
{
    return offsetBytes(header, -header->frontPadSize);
}

size_t toBaseSize(const LargeChunkHeader *const header)
{
    return header->frontPadSize + LARGE_CHUNK_HEADER_SIZE + header->payloadSize;
}

LargeChunkHeader *findLargeChunk(const void *const mem)
{
    const LargeChunkHeader *current = zones.large;
    while (current != NULL)
    {
        if (largeChunk2Mem(current) == mem)
        {
            return (LargeChunkHeader *)current;
        }
        current = current->next;
    }
    return NULL;
}

LargeChunkHeader *toPrevious(const LargeChunkHeader *const original)
{
    LargeChunkHeader *previous = zones.large;
    while (previous != NULL)
    {
        if (previous->next == original)
        {
            return previous;
        }
        previous = previous->next;
    }
    return NULL;
}

void push(LargeChunkHeader *newbie)
{
    newbie->next = zones.large;
    zones.large = newbie;
}

bool pop(const LargeChunkHeader *const victim)
{
    const LargeChunkHeader *const found = findLargeChunk(largeChunk2Mem(victim));
    if (found != NULL)
    {
        LargeChunkHeader *const prev = toPrevious(victim);
        if (prev != NULL)
        {
            prev->next = found->next;
        }
        else
        {
            zones.large = found->next;
        }
        return true;
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
