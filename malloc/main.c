#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "../include/myMalloc.h"

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
void *myMalloc(const size_t size)
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
void myFree(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

#if DEBUG_MEMORY_LEAKS
    /* Untrack this allocation for leak detection */
    untrackAllocation(ptr);
#endif

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

/* Segfault handler for testing invalid memory access */
static void segfaultHandler(int sig)
{
    if (expectingSegfault)
    {
        longjmp(segfaultJmpBuf, 1);
    }
    else
    {
        /* Unexpected segfault, restore default behavior */
        signal(SIGSEGV, SIG_DFL);
        raise(SIGSEGV);
    }
}

/* Stress test - allocate and free many different sizes repeatedly */
static void stressTest(void)
{
    printf("\n===== STRESS TEST =====\n");
    const int testCount = 1000;
    const int maxAllocSize = 4096;

    /* Arrays to store allocated memory */
    void *allocations[testCount];
    size_t sizes[testCount];

    srand(time(NULL));

    /* First pass: allocate memory of different sizes */
    for (int i = 0; i < testCount; i++)
    {
        const size_t size = rand() % maxAllocSize + 1;
        allocations[i] = myMalloc(size);
        sizes[i] = size;

        /* Write to the memory to ensure it's usable */
        if (allocations[i])
        {
            memset(allocations[i], 0xAB, size);
        }
        else
        {
            printf("Failed to allocate %zu bytes\n", size);
        }
    }

    /* Free half the allocations */
    for (int i = 0; i < testCount; i += 2)
    {
        myFree(allocations[i]);
        allocations[i] = NULL;
    }

    /* Allocate new blocks to replace freed ones */
    for (int i = 0; i < testCount; i += 2)
    {
        const size_t size = rand() % maxAllocSize + 1;
        allocations[i] = myMalloc(size);
        sizes[i] = size;

        if (allocations[i])
        {
            memset(allocations[i], 0xCD, size);
        }
    }

    /* Free everything */
    for (int i = 0; i < testCount; i++)
    {
        myFree(allocations[i]);
    }

    printf("Stress test complete - allocated and freed %d blocks of memory\n", testCount);
}

/* Test edge cases like zero-sized allocations */
static void edgeCaseTests(void)
{
    printf("\n===== EDGE CASE TESTS =====\n");

    /* Test NULL free */
    printf("Testing myFree(NULL)... ");
    myFree(NULL); /* Should do nothing and not crash */
    printf("OK\n");

    /* Test zero allocation */
    printf("Testing myMalloc(0)... ");
    void *const zeroPtr = myMalloc(0);
    if (zeroPtr == NULL)
    {
        printf("OK - returned NULL as expected\n");
    }
    else
    {
        printf("FAIL - should return NULL for zero size\n");
        myFree(zeroPtr);
    }

    /* Test tiny allocation */
    printf("Testing tiny allocation (1 byte)... ");
    void *const tinyPtr = myMalloc(1);
    if (tinyPtr != NULL)
    {
        *((unsigned char *)tinyPtr) = 0xFF; /* Should be safe to write to */
        printf("OK\n");
        myFree(tinyPtr);
    }
    else
    {
        printf("FAIL - couldn't allocate 1 byte\n");
    }

    /* Test large allocation */
    printf("Testing large allocation (1MB)... ");
    const size_t largeSize = 1024 * 1024;
    void *const largePtr = myMalloc(largeSize);
    if (largePtr != NULL)
    {
        memset(largePtr, 0xAA, largeSize); /* Should be safe to write to */
        printf("OK\n");
        myFree(largePtr);
    }
    else
    {
        printf("FAIL - couldn't allocate 1MB\n");
    }
}

/* Test freeing invalid pointers */
static void invalidFreeTests(void)
{
    printf("\n===== INVALID FREE TESTS =====\n");

    /* Set up segfault handler */
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = segfaultHandler;
    sigaction(SIGSEGV, &sa, NULL);

    /* Test double free */
    printf("Testing double free... ");
    void *const ptr = myMalloc(100);
    if (ptr)
    {
        myFree(ptr);

        /* This is technically undefined behavior, but our implementation should handle it gracefully */
        expectingSegfault = true;
        if (setjmp(segfaultJmpBuf) == 0)
        {
            myFree(ptr); /* Second free of the same pointer */
            printf("OK - didn't crash on double free\n");
        }
        else
        {
            printf("FAIL - segfault on double free\n");
        }
        expectingSegfault = false;
    }
    else
    {
        printf("SKIP - failed to allocate memory for test\n");
    }

    /* Test freeing an invalid pointer */
    printf("Testing freeing invalid pointer... ");
    void *const badPtr = (void *)0x12345678;

    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0)
    {
        myFree(badPtr);
        printf("OK - didn't crash on invalid pointer\n");
    }
    else
    {
        printf("FAIL - segfault on invalid pointer\n");
    }
    expectingSegfault = false;
}

/* Test use-after-free detection */
static void useAfterFreeTest(void)
{
    printf("\n===== USE-AFTER-FREE TEST =====\n");

    /* Allocate some memory */
    printf("Allocating memory for UAF test... ");
    const size_t size = 1024;
    unsigned char *ptr = (unsigned char *)myMalloc(size);

    if (ptr)
    {
        /* Fill with recognizable pattern */
        for (size_t i = 0; i < size; i++)
        {
            ptr[i] = (unsigned char)i;
        }
        printf("OK\n");

        /* Verify data */
        printf("Verifying data... ");
        bool dataValid = true;
        for (size_t i = 0; i < size; i++)
        {
            if (ptr[i] != (unsigned char)i)
            {
                dataValid = false;
                printf("Data corruption at offset %zu\n", i);
                break;
            }
        }

        if (dataValid)
        {
            printf("OK\n");
        }

        /* Free the memory */
        printf("Freeing memory... ");
        myFree(ptr);
        printf("OK\n");

        /* Try to use the memory after freeing (may or may not cause issues) */
        printf("Testing use-after-free (note: behavior is undefined!)... ");
        struct sigaction sa;
        memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = segfaultHandler;
        sigaction(SIGSEGV, &sa, NULL);

        expectingSegfault = true;
        if (setjmp(segfaultJmpBuf) == 0)
        {
            /* Try to write to freed memory - this is undefined behavior */
            ptr[0] = 0xFF;
            ptr[size / 2] = 0xFF;
            ptr[size - 1] = 0xFF;

            printf("Memory still accessible after free (expected behavior for our allocator)\n");
        }
        else
        {
            printf("Segfault when accessing freed memory\n");
        }
        expectingSegfault = false;
    }
    else
    {
        printf("SKIP - failed to allocate memory for test\n");
    }
}

/* Test memory exhaustion */
static void exhaustionTest(void)
{
    printf("\n===== MEMORY EXHAUSTION TEST =====\n");
    printf("Note: This test will try to allocate excessive memory\n");

    /* Try to allocate increasingly large blocks until failure */
    const size_t initialSize = 1024 * 1024; /* 1 MB */
    size_t size = initialSize;
    size_t totalAllocated = 0;
    int allocCount = 0;

    while (1)
    {
        printf("Trying to allocate %zu bytes... ", size);
        void *ptr = myMalloc(size);

        if (ptr)
        {
            /* Write to the memory to ensure it's usable */
            memset(ptr, 0xAA, size);

            totalAllocated += size;
            allocCount++;
            printf("OK (total: %zu bytes in %d blocks)\n", totalAllocated, allocCount);

            /* Grow allocation size exponentially */
            size *= 2;

            /* Don't free the memory - we want to exhaust the address space */
        }
        else
        {
            printf("Failed - memory exhaustion successful after %zu bytes\n", totalAllocated);
            break;
        }

        /* Safety limit to prevent test from running too long */
        if (allocCount >= 20)
        {
            printf("Stopping test after %d allocations\n", allocCount);
            break;
        }
    }
}

/* Test fragmentation resistance */
static void fragmentationTest(void)
{
    printf("\n===== FRAGMENTATION TEST =====\n");

    const int blockCount = 1000;
    const size_t blockSize = 128;

    /* Arrays to store allocated memory */
    void *blocks[blockCount];

    /* First, allocate many blocks of the same size */
    printf("Allocating %d blocks of %zu bytes each... ", blockCount, blockSize);
    for (int i = 0; i < blockCount; i++)
    {
        blocks[i] = myMalloc(blockSize);
        if (!blocks[i])
        {
            printf("Failed at block %d\n", i);
            break;
        }

        /* Write unique pattern to each block */
        memset(blocks[i], (unsigned char)i, blockSize);
    }
    printf("OK\n");

    /* Free every other block to create fragmentation */
    printf("Freeing every other block to create fragmentation... ");
    for (int i = 0; i < blockCount; i += 2)
    {
        myFree(blocks[i]);
        blocks[i] = NULL;
    }
    printf("OK\n");

    /* Try to allocate blocks larger than the original size */
    printf("Allocating blocks larger than the freed spaces... ");
    size_t largerSize = blockSize * 3;
    int successCount = 0;

    for (int i = 0; i < blockCount; i += 2)
    {
        blocks[i] = myMalloc(largerSize);
        if (blocks[i])
        {
            successCount++;
            /* Mark the allocation with recognizable pattern */
            memset(blocks[i], 0xBB, largerSize);
        }
    }

    printf("Successfully allocated %d/%d larger blocks\n", successCount, blockCount / 2);

    /* Verify that the odd-indexed blocks still have their original content */
    printf("Verifying integrity of unfreed blocks... ");
    int corruptCount = 0;

    for (int i = 1; i < blockCount; i += 2)
    {
        if (blocks[i])
        {
            unsigned char *data = (unsigned char *)blocks[i];
            bool isCorrupt = false;

            for (size_t j = 0; j < blockSize; j++)
            {
                if (data[j] != (unsigned char)i)
                {
                    isCorrupt = true;
                    break;
                }
            }

            if (isCorrupt)
            {
                corruptCount++;
            }
        }
    }

    if (corruptCount == 0)
    {
        printf("All blocks intact - no corruption detected\n");
    }
    else
    {
        printf("Found %d corrupted blocks\n", corruptCount);
    }

    /* Free all blocks */
    printf("Freeing all blocks... ");
    for (int i = 0; i < blockCount; i++)
    {
        myFree(blocks[i]);
    }
    printf("OK\n");
}

int main(void)
{
    printf("Memory Allocator Test Suite\n");
    printf("==========================\n");

    /* Run tests */
    stressTest();
    edgeCaseTests();
    invalidFreeTests();
    useAfterFreeTest();
    fragmentationTest();

    /* Uncomment to run exhaustion test (may affect system stability) */
    /* exhaustionTest(); */

    /* Print memory statistics */
    printMemoryStats();

    /* Print memory leak report */
    printMemoryLeakReport();

    return 0;
}
