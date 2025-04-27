#include "all.h"

Zones zones;

__attribute__((constructor)) void initializePreAllocatedZones()
{
    // Allocate memory for the management struct itself using mmap
    const AllocResult tinyZoneResult = allocateMemory(TINY_ZONE_SIZE);
    if (tinyZoneResult.succeeded)
    {
        // Initialize the tiny zone
        zones.tiny.base = tinyZoneResult.allocatedMemoryAddress;
        zones.tiny.zoneSize = TINY_ZONE_SIZE;
        zones.tiny.frontPadSize = distanceToNextAlignment(tinyZoneResult.allocatedMemoryAddress);

        const AllocResult smallZoneResult = allocateMemory(SMALL_ZONE_SIZE);
        if (smallZoneResult.succeeded)
        {
            // Initialize the small zone
            zones.small.base = smallZoneResult.allocatedMemoryAddress;
            zones.small.zoneSize = SMALL_ZONE_SIZE;
            zones.small.frontPadSize = distanceToNextAlignment(smallZoneResult.allocatedMemoryAddress);
        }
        else
        {
            perror("Failed to allocate memory for the small zone in constructor");
        }
    }
    else
    {
        perror("Failed to allocate memory for the management struct in constructor");
    }
}

__attribute__((destructor)) void cleanupPreAllocatedZones()
{
    if (zones.tiny.base != NULL)
    {
        unallocateMemory(zones.tiny.base, zones.tiny.zoneSize);
    }
    if (zones.small.base != NULL)
    {
        unallocateMemory(zones.small.base, zones.small.zoneSize);
    }
}
