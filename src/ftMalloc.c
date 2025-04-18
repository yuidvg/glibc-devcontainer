#include "../include/all.h"

/* Ensure MAP_ANONYMOUS is defined - fix platform differences */
#ifndef MAP_ANONYMOUS
#ifdef __APPLE__
#define MAP_ANONYMOUS MAP_ANON
#else
#define MAP_ANONYMOUS 0x20 /* Value on Linux systems */
#endif
#endif

/* Constants for memory allocation categories */
#define TINY_MAX_SIZE 128   /* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024 /* m: max size for small allocations */

/* Size of allocation zones - tuned for at least 100 allocations per zone */
#define TINY_ZONE_SIZE (getPageSize() * 16)   /* N: Size for tiny zones */
#define SMALL_ZONE_SIZE (getPageSize() * 128) /* M: Size for small zones */

/* Debug settings */
#define DEBUG_MEMORY_LEAKS 1 /* Whether to track allocations for leak detection */
#define DEBUG_VERBOSE 0      /* Whether to print verbose debug info */

/* Metadata structure for memory blocks - placed before actual memory */
typedef struct
{
    size_t size;     /* Size requested by user */
    size_t realSize; /* Actual size including header */
    bool isFree;     /* Whether this block is free */
} BlockHeader;

/* Zone types */
typedef enum
{
    ZONE_TINY,
    ZONE_SMALL,
    ZONE_LARGE
} ZoneType;

/* Linked list of memory zones */
typedef struct Zone
{
    ZoneType type;
    size_t size;
    struct Zone *next;
} Zone;

/* Global state */
static Zone *tinyZones = NULL;
static Zone *smallZones = NULL;
static Zone *largeZones = NULL;

/* For catching segfaults in tests */
static jmp_buf segfaultJmpBuf;
static bool expectingSegfault = false;

/* Memory leak detection */
#if DEBUG_MEMORY_LEAKS
typedef struct AllocRecord
{
    void *ptr;
    size_t size;
    struct AllocRecord *next;
} AllocRecord;

static AllocRecord *allocations = NULL;
static size_t totalAllocated = 0;
static size_t peakAllocated = 0;
static size_t allocCount = 0;
static size_t freeCount = 0;

/* Track a new allocation */
static void trackAllocation(void *ptr, size_t size)
{
    if (!ptr)
        return;

    AllocRecord *record = malloc(sizeof(AllocRecord));
    if (!record)
        return; /* Malloc for tracking failed - just continue */

    record->ptr = ptr;
    record->size = size;
    record->next = allocations;
    allocations = record;

    totalAllocated += size;
    allocCount++;

    if (totalAllocated > peakAllocated)
    {
        peakAllocated = totalAllocated;
    }

    if (DEBUG_VERBOSE)
    {
        printf("ALLOC: %p, size: %zu\n", ptr, size);
    }
}

/* Remove an allocation from tracking */
static void untrackAllocation(void *ptr)
{
    if (!ptr)
        return;

    AllocRecord *current = allocations;
    AllocRecord *prev = NULL;

    while (current)
    {
        if (current->ptr == ptr)
        {
            if (prev)
            {
                prev->next = current->next;
            }
            else
            {
                allocations = current->next;
            }

            totalAllocated -= current->size;
            freeCount++;

            if (DEBUG_VERBOSE)
            {
                printf("FREE: %p, size: %zu\n", ptr, current->size);
            }

            free(current);
            return;
        }

        prev = current;
        current = current->next;
    }

    /* If we get here, the pointer wasn't found */
    if (DEBUG_VERBOSE)
    {
        printf("WARNING: Trying to free untracked pointer %p\n", ptr);
    }
}

/* Print memory leak report */
static void printMemoryLeakReport(void)
{
    printf("\n===== MEMORY LEAK REPORT =====\n");
    printf("Peak memory usage: %zu bytes\n", peakAllocated);
    printf("Allocations: %zu, Frees: %zu, Difference: %zu\n", allocCount, freeCount, allocCount - freeCount);

    if (allocations)
    {
        printf("MEMORY LEAKS DETECTED: %zu bytes still allocated\n", totalAllocated);

        /* Print each leaked allocation */
        AllocRecord *current = allocations;
        int count = 0;
        while (current && count < 10)
        { /* Limit to 10 leaks in the report */
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
}
#endif

/* Result type for allocation functions */
typedef struct
{
    bool succeeded;
    void *ptr;
} ResultAlloc;

/* Get system page size (immutable) */
static size_t getPageSize(void)
{
    const size_t pageSize = sysconf(_SC_PAGESIZE);
    return pageSize;
}

/* Check if a size fits in a zone type */
static ZoneType getZoneTypeForSize(const size_t size)
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
static size_t getZoneSizeForType(const ZoneType type)
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
static ResultAlloc createZone(const ZoneType type, const size_t requestedSize)
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
    void *const mmapResult = mmap(NULL, zoneSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mmapResult == MAP_FAILED)
    {
        return (ResultAlloc){.succeeded = false, .ptr = NULL};
    }

    /* Initialize zone header */
    Zone *const newZone = (Zone *)mmapResult;
    newZone->type = type;
    newZone->size = zoneSize;
    newZone->next = NULL;

    /* Initialize first block header (entire zone is free except headers) */
    BlockHeader *const firstBlock = (BlockHeader *)(newZone + 1);
    const size_t availableSize = zoneSize - sizeof(Zone) - sizeof(BlockHeader);

    firstBlock->size = (type == ZONE_LARGE) ? requestedSize : availableSize;
    firstBlock->realSize = availableSize;
    firstBlock->isFree = true;

    /* Add zone to appropriate list */
    if (type == ZONE_TINY)
    {
        Zone *const oldHead = tinyZones;
        tinyZones = newZone;
        newZone->next = oldHead;
    }
    else if (type == ZONE_SMALL)
    {
        Zone *const oldHead = smallZones;
        smallZones = newZone;
        newZone->next = oldHead;
    }
    else /* ZONE_LARGE */
    {
        Zone *const oldHead = largeZones;
        largeZones = newZone;
        newZone->next = oldHead;
    }

    return (ResultAlloc){.succeeded = true, .ptr = newZone};
}

/* Check if two blocks are adjacent and can be coalesced */
static bool areBlocksAdjacent(BlockHeader *first, BlockHeader *second)
{
    const char *firstEnd = (char *)(first + 1) + first->realSize;
    return (BlockHeader *)firstEnd == second;
}

/* Find a free block in existing zones using best-fit strategy */
static ResultAlloc findFreeBlock(const ZoneType type, const size_t size)
{
    Zone *currentZone = NULL;

    /* Best-fit variables */
    BlockHeader *bestFitBlock = NULL;
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
        return (ResultAlloc){.succeeded = false, .ptr = NULL};
    }

    /* Walk through zones of this type */
    while (currentZone != NULL)
    {
        /* Start at first block in zone */
        BlockHeader *currentBlock = (BlockHeader *)(currentZone + 1);
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
                        return (ResultAlloc){.succeeded = true, .ptr = bestFitBlock};
                    }
                }
            }

            /* Move to next block */
            currentBlock = (BlockHeader *)((char *)currentBlock + currentBlock->realSize + sizeof(BlockHeader));

            /* Safety check */
            if ((size_t)currentBlock >= zoneEnd)
            {
                break;
            }
        }

        currentZone = currentZone->next;
    }

    /* Return best fit if found */
    if (bestFitBlock != NULL)
    {
        return (ResultAlloc){.succeeded = true, .ptr = bestFitBlock};
    }

    /* No suitable free block found */
    return (ResultAlloc){.succeeded = false, .ptr = NULL};
}

/* Split a block if it's significantly larger than needed */
static void *splitBlockIfNeeded(BlockHeader *block, const size_t requestedSize)
{
    const size_t minSplitSize = sizeof(BlockHeader) + 16; /* Minimum practical size */
    const size_t currentSize = block->realSize;
    const size_t headerSize = sizeof(BlockHeader);

    if (currentSize >= requestedSize + headerSize + minSplitSize)
    {
        /* Enough space to split - create new block after this one */
        const size_t remainingSize = currentSize - requestedSize - headerSize;
        BlockHeader *const newBlock = (BlockHeader *)((char *)(block + 1) + requestedSize);

        /* Initialize the new block */
        newBlock->size = remainingSize - headerSize;
        newBlock->realSize = remainingSize - headerSize;
        newBlock->isFree = true;

        /* Update current block */
        block->size = requestedSize;
        block->realSize = requestedSize;
        block->isFree = false;
    }
    else
    {
        /* Not worth splitting - just mark as used */
        block->size = requestedSize;
        block->realSize = currentSize;
        block->isFree = false;
    }

    return (void *)(block + 1);
}

/* Coalesce adjacent free blocks to reduce fragmentation */
static void coalesceZone(Zone *zone)
{
    if (!zone)
        return;

    const size_t zoneEnd = (size_t)zone + zone->size;
    BlockHeader *currentBlock = (BlockHeader *)(zone + 1);

    while ((size_t)currentBlock < zoneEnd)
    {
        /* Skip if not free */
        if (!currentBlock->isFree)
        {
            currentBlock = (BlockHeader *)((char *)currentBlock + currentBlock->realSize + sizeof(BlockHeader));
            continue;
        }

        /* Try to find next block */
        BlockHeader *nextBlock = (BlockHeader *)((char *)currentBlock + currentBlock->realSize + sizeof(BlockHeader));

        /* If next block is past end or not free, move on */
        if ((size_t)nextBlock >= zoneEnd || !nextBlock->isFree)
        {
            currentBlock = nextBlock;
            continue;
        }

        /* Merge the blocks */
        const size_t newSize = currentBlock->realSize + sizeof(BlockHeader) + nextBlock->realSize;
        currentBlock->realSize = newSize;

        /* Continue from current block to check for more merges */
    }
}

/* The main memory allocation function */
void *ftMalloc(const size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    /* Determine zone type based on size */
    const ZoneType zoneType = getZoneTypeForSize(size);

    /* Try to find a free block in existing zones */
    const ResultAlloc freeBlockResult = findFreeBlock(zoneType, size);

    void *result = NULL;

    if (freeBlockResult.succeeded)
    {
        /* Found a free block - use it */
        result = splitBlockIfNeeded((BlockHeader *)freeBlockResult.ptr, size);
    }
    else
    {
        /* Need to create a new zone */
        const ResultAlloc newZoneResult = createZone(zoneType, size);

        if (!newZoneResult.succeeded)
        {
            /* Failed to create zone */
            return NULL;
        }

        /* For large allocations, return memory right after the first block header */
        if (zoneType == ZONE_LARGE)
        {
            BlockHeader *const blockHeader = (BlockHeader *)((Zone *)newZoneResult.ptr + 1);

            /* Mark as used */
            blockHeader->size = size;
            blockHeader->isFree = false;

            result = (void *)(blockHeader + 1);
        }
        else
        {
            /* For small/tiny, the first block in the new zone is guaranteed to be big enough */
            BlockHeader *const firstBlock = (BlockHeader *)((Zone *)newZoneResult.ptr + 1);
            result = splitBlockIfNeeded(firstBlock, size);
        }
    }

    /* Track allocation if debugging is enabled */
#if DEBUG_MEMORY_LEAKS
    if (result)
    {
        trackAllocation(result, size);
    }
#endif

    return result;
}

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

/* Function to help debug and print memory usage */
static void printMemoryStats(void)
{
    size_t tinyCount = 0;
    size_t smallCount = 0;
    size_t largeCount = 0;
    size_t totalAllocated = 0;

    /* Count tiny zones */
    Zone *zone = tinyZones;
    while (zone != NULL)
    {
        tinyCount++;
        totalAllocated += zone->size;
        zone = zone->next;
    }

    /* Count small zones */
    zone = smallZones;
    while (zone != NULL)
    {
        smallCount++;
        totalAllocated += zone->size;
        zone = zone->next;
    }

    /* Count large zones */
    zone = largeZones;
    while (zone != NULL)
    {
        largeCount++;
        totalAllocated += zone->size;
        zone = zone->next;
    }

    printf("Memory zones: Tiny=%zu, Small=%zu, Large=%zu\n", tinyCount, smallCount, largeCount);
    printf("Total memory allocated: %zu bytes\n", totalAllocated);
}
