#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "../include/myMallocInternal.h"

/* Global state */
static Zone* tinyZones = NULL;
static Zone* smallZones = NULL;
static Zone* largeZones = NULL;

/* Memory leak detection */
#if DEBUG_MEMORY_LEAKS
typedef struct {
    const void* ptr;
    const size_t size;
    struct AllocRecord* next;
} AllocRecord;

static AllocRecord* allocations = NULL;
static size_t totalAllocated = 0;
static size_t peakAllocated = 0;
static size_t allocCount = 0;
static size_t freeCount = 0;

/* Track a new allocation */
void trackAllocation(const void* ptr, const size_t size) {
    if (!ptr) return;

    const AllocRecord* record = malloc(sizeof(AllocRecord));
    if (!record) return;  /* Malloc for tracking failed - just continue */

    record->ptr = ptr;
    record->size = size;
    record->next = allocations;
    allocations = record;

    totalAllocated += size;
    allocCount++;

    if (totalAllocated > peakAllocated) {
        peakAllocated = totalAllocated;
    }

    if (DEBUG_VERBOSE) {
        printf("ALLOC: %p, size: %zu\n", ptr, size);
    }
}

/* Remove an allocation from tracking */
void untrackAllocation(const void* ptr) {
    if (!ptr) return;

    AllocRecord* current = allocations;
    AllocRecord* prev = NULL;

    while (current) {
        if (current->ptr == ptr) {
            if (prev) {
                prev->next = current->next;
            } else {
                allocations = current->next;
            }

            totalAllocated -= current->size;
            freeCount++;

            if (DEBUG_VERBOSE) {
                printf("FREE: %p, size: %zu\n", ptr, current->size);
            }

            free(current);
            return;
        }

        prev = current;
        current = current->next;
    }

    /* If we get here, the pointer wasn't found */
    if (DEBUG_VERBOSE) {
        printf("WARNING: Trying to free untracked pointer %p\n", ptr);
    }
}
#endif

/* Get system page size (immutable) */
size_t getPageSize(void)
{
    const size_t pageSize = sysconf(_SC_PAGESIZE);
    return pageSize;
}

/* Check if a size fits in a zone type */
ZoneType getZoneTypeForSize(const size_t size)
{
    if (size <= TINY_MAX_SIZE)
    {
        return ZONE_TINY;
    }
    else if (size <= SMALL_MAX_SIZE)
    {
        return ZONE_SMALL;
    }
    else
    {
        return ZONE_LARGE;
    }
}

/* Calculate required zone size based on zone type */
size_t getZoneSizeForType(const ZoneType type)
{
    if (type == ZONE_TINY)
    {
        return TINY_ZONE_SIZE;
    }
    else if (type == ZONE_SMALL)
    {
        return SMALL_ZONE_SIZE;
    }
    return 0; /* LARGE zones don't have a predefined size */
}

/* Create a new memory zone using mmap */
ResultAlloc createZone(const ZoneType type, const size_t requestedSize)
{
    size_t zoneSize = 0;

    if (type == ZONE_LARGE)
    {
        /* For large allocations, allocate exact size plus metadata */
        const size_t headerSize = sizeof(BlockHeader);
        const size_t zoneHeaderSize = sizeof(Zone);
        const size_t pageSize = getPageSize();

        /* Round up to nearest multiple of page size */
        const size_t totalSize = headerSize + zoneHeaderSize + requestedSize;
        zoneSize = (totalSize + pageSize - 1) / pageSize * pageSize;
    }
    else
    {
        zoneSize = getZoneSizeForType(type);
    }

    /* Use mmap to allocate memory directly from OS */
    void* const mmapResult = mmap(
        NULL,
        zoneSize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

    if (mmapResult == MAP_FAILED)
    {
        return (ResultAlloc){
            .succeeded = false,
            .ptr = NULL
        };
    }

    /* Initialize zone header */
    Zone* const newZone = (Zone*)mmapResult;
    newZone->type = type;
    newZone->size = zoneSize;
    newZone->next = NULL;

    /* Initialize first block header (entire zone is free except headers) */
    BlockHeader* const firstBlock = (BlockHeader*)(newZone + 1);
    const size_t availableSize = zoneSize - sizeof(Zone) - sizeof(BlockHeader);

    firstBlock->size = (type == ZONE_LARGE) ? requestedSize : availableSize;
    firstBlock->realSize = availableSize;
    firstBlock->isFree = true;

    /* Add zone to appropriate list */
    if (type == ZONE_TINY)
    {
        Zone* const oldHead = tinyZones;
        tinyZones = newZone;
        newZone->next = oldHead;
    }
    else if (type == ZONE_SMALL)
    {
        Zone* const oldHead = smallZones;
        smallZones = newZone;
        newZone->next = oldHead;
    }
    else /* ZONE_LARGE */
    {
        Zone* const oldHead = largeZones;
        largeZones = newZone;
        newZone->next = oldHead;
    }

    return (ResultAlloc){
        .succeeded = true,
        .ptr = newZone
    };
}

/* Find a free block in existing zones using best-fit strategy */
ResultAlloc findFreeBlock(const ZoneType type, const size_t size)
{
    Zone* currentZone = NULL;

    /* Best-fit variables */
    BlockHeader* bestFitBlock = NULL;
    size_t bestFitSize = SIZE_MAX;

    if (type == ZONE_TINY)
    {
        currentZone = tinyZones;
    }
    else if (type == ZONE_SMALL)
    {
        currentZone = smallZones;
    }
    else
    {
        /* Large allocations don't reuse blocks */
        return (ResultAlloc){ .succeeded = false, .ptr = NULL };
    }

    /* Walk through zones of this type */
    while (currentZone != NULL)
    {
        /* Start at first block in zone */
        BlockHeader* currentBlock = (BlockHeader*)(currentZone + 1);
        const size_t zoneEnd = (size_t)currentZone + currentZone->size;

        /* Walk through blocks in this zone */
        while ((size_t)currentBlock < zoneEnd)
        {
            if (currentBlock->isFree && currentBlock->realSize >= size)
            {
                /* Found usable block - check if it's a better fit than current best */
                if (currentBlock->realSize < bestFitSize)
                {
                    bestFitBlock = currentBlock;
                    bestFitSize = currentBlock->realSize;

                    /* If perfect fit, return immediately */
                    if (bestFitSize == size)
                    {
                        return (ResultAlloc){
                            .succeeded = true,
                            .ptr = bestFitBlock
                        };
                    }
                }
            }

            /* Move to next block */
            const size_t blockSize = currentBlock->realSize;
            currentBlock = (BlockHeader*)((size_t)currentBlock + blockSize + sizeof(BlockHeader));
        }

        currentZone = currentZone->next;
    }

    /* Return best fit if found, otherwise failure */
    if (bestFitBlock != NULL)
    {
        return (ResultAlloc){
            .succeeded = true,
            .ptr = bestFitBlock
        };
    }
    else
    {
        return (ResultAlloc){
            .succeeded = false,
            .ptr = NULL
        };
    }
}

/* Split a block if it's significantly larger than needed */
void* splitBlockIfNeeded(BlockHeader* block, const size_t requestedSize)
{
    const size_t minSplitSize = 2 * sizeof(BlockHeader); /* Minimum size to split */
    const size_t totalSize = block->realSize;

    /* Check if we can split this block */
    if (totalSize >= requestedSize + minSplitSize)
    {
        /* Calculate size of the split */
        const size_t splitSize = totalSize - requestedSize - sizeof(BlockHeader);

        /* Update current block */
        block->size = requestedSize;
        block->realSize = requestedSize;
        block->isFree = false;

        /* Create new block in the split space */
        BlockHeader* const newBlock = (BlockHeader*)((size_t)block + sizeof(BlockHeader) + requestedSize);
        newBlock->size = splitSize;
        newBlock->realSize = splitSize;
        newBlock->isFree = true;
    }
    else
    {
        /* Block not large enough to split, just use it as is */
        block->size = requestedSize;
        block->isFree = false;
    }

    /* Return pointer to usable memory (after header) */
    return (void*)((size_t)block + sizeof(BlockHeader));
}

void coalesceZone(Zone* zone)
{
    if (!zone) return;

    /* Start at first block in zone */
    BlockHeader* current = (BlockHeader*)(zone + 1);
    const size_t zoneEnd = (size_t)zone + zone->size;

    while ((size_t)current < zoneEnd)
    {
        /* If current block is free, check if next block is also free */
        if (current->isFree)
        {
            /* Calculate address of next block */
            BlockHeader* const next = (BlockHeader*)((size_t)current + sizeof(BlockHeader) + current->realSize);

            /* If next block is within zone bounds and is free, merge them */
            if ((size_t)next < zoneEnd && next->isFree)
            {
                /* Combine sizes including next block's header */
                const size_t combinedSize = current->realSize + sizeof(BlockHeader) + next->realSize;
                current->realSize = combinedSize;

                /* Skip to next iteration without advancing current pointer */
                continue;
            }
        }

        /* Move to next block */
        current = (BlockHeader*)((size_t)current + sizeof(BlockHeader) + current->realSize);
    }
}

void* myMalloc(const size_t size)
{
    if (size == 0) return NULL;

    /* Determine zone type based on allocation size */
    const ZoneType zoneType = getZoneTypeForSize(size);

    /* Try to find a free block in existing zones */
    const ResultAlloc findResult = findFreeBlock(zoneType, size);

    if (findResult.succeeded)
    {
        /* Found existing block - split if needed */
        BlockHeader* const block = (BlockHeader*)findResult.ptr;
        void* const result = splitBlockIfNeeded(block, size);

        #if DEBUG_MEMORY_LEAKS
        trackAllocation(result, size);
        #endif

        return result;
    }
    else
    {
        /* No suitable block found - create new zone */
        const ResultAlloc zoneResult = createZone(zoneType, size);

        if (!zoneResult.succeeded)
        {
            /* Failed to create zone */
            return NULL;
        }

        /* Get first block in the new zone */
        Zone* const newZone = (Zone*)zoneResult.ptr;
        BlockHeader* const block = (BlockHeader*)(newZone + 1);

        /* Mark block as allocated and return pointer to usable memory */
        block->isFree = false;

        void* const result = (void*)((size_t)block + sizeof(BlockHeader));

        #if DEBUG_MEMORY_LEAKS
        trackAllocation(result, size);
        #endif

        return result;
    }
}

void myFree(void* ptr)
{
    if (!ptr) return;

    /* Find the block header for this pointer */
    BlockHeader* const block = (BlockHeader*)((size_t)ptr - sizeof(BlockHeader));

    /* Mark the block as free */
    block->isFree = true;

    /* Find which zone this belongs to */
    Zone* zone = NULL;
    Zone* current = tinyZones;

    while (current)
    {
        const size_t zoneStart = (size_t)current;
        const size_t zoneEnd = zoneStart + current->size;

        if ((size_t)block >= zoneStart && (size_t)block < zoneEnd)
        {
            zone = current;
            break;
        }

        current = current->next;
    }

    if (!zone)
    {
        /* Not found in tiny zones, check small zones */
        current = smallZones;

        while (current)
        {
            const size_t zoneStart = (size_t)current;
            const size_t zoneEnd = zoneStart + current->size;

            if ((size_t)block >= zoneStart && (size_t)block < zoneEnd)
            {
                zone = current;
                break;
            }

            current = current->next;
        }
    }

    if (!zone)
    {
        /* Not found in small zones either, must be in large zones */
        current = largeZones;

        while (current)
        {
            const size_t zoneStart = (size_t)current;
            const size_t zoneEnd = zoneStart + current->size;

            if ((size_t)block >= zoneStart && (size_t)block < zoneEnd)
            {
                zone = current;
                break;
            }

            current = current->next;
        }
    }

    /* Coalesce free blocks in this zone to reduce fragmentation */
    if (zone)
    {
        coalesceZone(zone);
    }

    #if DEBUG_MEMORY_LEAKS
    untrackAllocation(ptr);
    #endif
}

void printMemoryStats(void)
{
    printf("\n===== MEMORY ALLOCATOR STATS =====\n");

    size_t totalMemory = 0;
    size_t usedMemory = 0;
    size_t freeMemory = 0;

    /* Count TINY zones */
    size_t tinyZoneCount = 0;
    Zone* current = tinyZones;
    while (current)
    {
        tinyZoneCount++;
        totalMemory += current->size;
        current = current->next;
    }

    /* Count SMALL zones */
    size_t smallZoneCount = 0;
    current = smallZones;
    while (current)
    {
        smallZoneCount++;
        totalMemory += current->size;
        current = current->next;
    }

    /* Count LARGE zones */
    size_t largeZoneCount = 0;
    current = largeZones;
    while (current)
    {
        largeZoneCount++;
        totalMemory += current->size;
        current = current->next;
    }

    printf("Total memory mapped: %zu bytes\n", totalMemory);
    printf("TINY zones: %zu\n", tinyZoneCount);
    printf("SMALL zones: %zu\n", smallZoneCount);
    printf("LARGE zones: %zu\n", largeZoneCount);

    #if DEBUG_MEMORY_LEAKS
    printf("Current allocations: %zu\n", allocCount - freeCount);
    #endif

    printf("================================\n");
}

void printMemoryLeakReport(void)
{
    #if DEBUG_MEMORY_LEAKS
    printf("\n===== MEMORY LEAK REPORT =====\n");
    printf("Peak memory usage: %zu bytes\n", peakAllocated);
    printf("Allocations: %zu, Frees: %zu, Difference: %zu\n",
           allocCount, freeCount, allocCount - freeCount);

    if (allocations)
    {
        printf("MEMORY LEAKS DETECTED: %zu bytes still allocated\n", totalAllocated);

        /* Print each leaked allocation */
        AllocRecord* current = allocations;
        int count = 0;
        while (current && count < 10)
        {  /* Limit to 10 leaks in the report */
            printf("  Leak %d: %p, size: %zu\n", count + 1, current->ptr, current->size);
            current = current->next;
            count++;
        }

        if (current)
        {
            printf("  ... and more leaks\n");
        }
    }
    else
    {
        printf("No memory leaks detected!\n");
    }
    printf("==============================\n");
    #else
    printf("Memory leak detection is disabled.\n");
    #endif
}