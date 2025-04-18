#include "all.h"

/* Free memory previously allocated with myMalloc */
void ftFree(void *ptr)
{
    if (ptr != NULL)
    {
        /* Get block header - it's right before the user's pointer */
        BlockHeader *const blockHeader = ((BlockHeader *)ptr) - 1;

        /* Mark the block as free */
        blockHeader->isFree = true;

        /* For large allocations, consider unmapping directly */
        /* First find which zone this belongs to */
        Zone *prevZone = NULL;
        Zone *currentZone = largeZones;

        while (currentZone != NULL)
        {
            const size_t zoneStart = (size_t)currentZone;
            const size_t zoneEnd = zoneStart + currentZone->size;

            if ((size_t)blockHeader >= zoneStart && (size_t)blockHeader < zoneEnd)
            {
                /* This block is part of a large zone - unmap it */
                if (prevZone == NULL)
                {
                    largeZones = currentZone->next;
                }
                else
                {
                    prevZone->next = currentZone->next;
                }

                munmap(currentZone, currentZone->size);
                return;
            }

            prevZone = currentZone;
            currentZone = currentZone->next;
        }

        /* For tiny zones, coalesce adjacent free blocks */
        currentZone = tinyZones;
        while (currentZone != NULL)
        {
            const size_t zoneStart = (size_t)currentZone;
            const size_t zoneEnd = zoneStart + currentZone->size;

            if ((size_t)blockHeader >= zoneStart && (size_t)blockHeader < zoneEnd)
            {
                coalesceZone(currentZone);
                return;
            }

            currentZone = currentZone->next;
        }

        /* For small zones, also coalesce adjacent free blocks */
        currentZone = smallZones;
        while (currentZone != NULL)
        {
            const size_t zoneStart = (size_t)currentZone;
            const size_t zoneEnd = zoneStart + currentZone->size;

            if ((size_t)blockHeader >= zoneStart && (size_t)blockHeader < zoneEnd)
            {
                coalesceZone(currentZone);
                return;
            }

            currentZone = currentZone->next;
        }
    }
}