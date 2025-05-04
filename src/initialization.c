#include "all.h"

Zones zones;

__attribute__((constructor)) void initializePreAllocatedZones()
{
    zones.large = NULL;
    // Allocate memory for the management struct itself using mmap
    const AllocResult tinyZoneResult = allocateMemory(TINY_ZONE_SIZE);
    if (tinyZoneResult.succeeded)
    {
        // Initialize the tiny zone
        zones.tiny.base = tinyZoneResult.allocatedMemoryAddress;
        zones.tiny.baseSize = TINY_ZONE_SIZE;
        zones.tiny.frontPadSize = distanceToNextAlignment(tinyZoneResult.allocatedMemoryAddress);
        ChunkHeader *const firstChunkInTinyZone = toFirstChunk(&zones.tiny);
        firstChunkInTinyZone->payloadSize = zones.tiny.baseSize - zones.tiny.frontPadSize - CHUNK_HEADER_SIZE;
        firstChunkInTinyZone->isFree = true;

        const AllocResult smallZoneResult = allocateMemory(SMALL_ZONE_SIZE);
        if (smallZoneResult.succeeded)
        {
            // Initialize the small zone
            zones.small.base = smallZoneResult.allocatedMemoryAddress;
            zones.small.baseSize = SMALL_ZONE_SIZE;
            zones.small.frontPadSize = distanceToNextAlignment(smallZoneResult.allocatedMemoryAddress);
            ChunkHeader *const firstChunkInSmallZone = toFirstChunk(&zones.small);
            firstChunkInSmallZone->payloadSize = zones.small.baseSize - zones.small.frontPadSize - CHUNK_HEADER_SIZE;
            firstChunkInSmallZone->isFree = true;
        }
        else
        {
            printError("Failed to allocate memory for the small zone in constructor");
        }
    }
    else
    {
        printError("Failed to allocate memory for the management struct in constructor");
    }
}

__attribute__((destructor)) void cleanupPreAllocatedZones()
{
    if (zones.tiny.base != NULL)
    {
        unallocateMemory(zones.tiny.base, zones.tiny.baseSize);
    }
    if (zones.small.base != NULL)
    {
        unallocateMemory(zones.small.base, zones.small.baseSize);
    }
}
