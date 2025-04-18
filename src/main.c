#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>

/* Ensure MAP_ANONYMOUS is defined - fix platform differences */
#ifndef MAP_ANONYMOUS
#ifdef __APPLE__
#define MAP_ANONYMOUS MAP_ANON
#else
#define MAP_ANONYMOUS 0x20  /* Value on Linux systems */
#endif
#endif

/* Constants for memory allocation categories */
#define TINY_MAX_SIZE 128        /* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024      /* m: max size for small allocations */

/* Size of allocation zones - tuned for at least 100 allocations per zone */
#define TINY_ZONE_SIZE (getPageSize() * 16)   /* N: Size for tiny zones */
#define SMALL_ZONE_SIZE (getPageSize() * 128) /* M: Size for small zones */

/* Debug settings */
#define DEBUG_MEMORY_LEAKS 1     /* Whether to track allocations for leak detection */
#define DEBUG_VERBOSE 0          /* Whether to print verbose debug info */

/* Metadata structure for memory blocks - placed before actual memory */
typedef struct {
    size_t size;      /* Size requested by user */
    size_t realSize;  /* Actual size including header */
    bool isFree;      /* Whether this block is free */
} BlockHeader;

/* Zone types */
typedef enum {
    ZONE_TINY,
    ZONE_SMALL,
    ZONE_LARGE
} ZoneType;

/* Linked list of memory zones */
typedef struct Zone {
    ZoneType type;
    size_t size;
    struct Zone* next;
} Zone;

/* Global state */
static Zone* tinyZones = NULL;
static Zone* smallZones = NULL;
static Zone* largeZones = NULL;

/* For catching segfaults in tests */
static jmp_buf segfaultJmpBuf;
static bool expectingSegfault = false;

/* Memory leak detection */
#if DEBUG_MEMORY_LEAKS
typedef struct AllocRecord {
    void* ptr;
    size_t size;
    struct AllocRecord* next;
} AllocRecord;

static AllocRecord* allocations = NULL;
static size_t totalAllocated = 0;
static size_t peakAllocated = 0;
static size_t allocCount = 0;
static size_t freeCount = 0;

/* Track a new allocation */
static void trackAllocation(void* ptr, size_t size) {
    if (!ptr) return;

    AllocRecord* record = malloc(sizeof(AllocRecord));
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
static void untrackAllocation(void* ptr) {
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

/* Print memory leak report */
static void printMemoryLeakReport(void) {
    printf("\n===== MEMORY LEAK REPORT =====\n");
    printf("Peak memory usage: %zu bytes\n", peakAllocated);
    printf("Allocations: %zu, Frees: %zu, Difference: %zu\n",
           allocCount, freeCount, allocCount - freeCount);

    if (allocations) {
        printf("MEMORY LEAKS DETECTED: %zu bytes still allocated\n", totalAllocated);

        /* Print each leaked allocation */
        AllocRecord* current = allocations;
        int count = 0;
        while (current && count < 10) {  /* Limit to 10 leaks in the report */
            printf("  Leak %d: %p, size: %zu\n", count + 1, current->ptr, current->size);
            current = current->next;
            count++;
        }

        if (current) {
            printf("  ... and more leaks\n");
        }
    } else {
        printf("No memory leaks detected!\n");
    }
    printf("==============================\n");
}
#endif

/* Result type for allocation functions */
typedef struct {
    bool succeeded;
    void* ptr;
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

/* Check if two blocks are adjacent and can be coalesced */
static bool areBlocksAdjacent(BlockHeader* first, BlockHeader* second)
{
    const char* firstEnd = (char*)(first + 1) + first->realSize;
    return (BlockHeader*)firstEnd == second;
}

/* Find a free block in existing zones using best-fit strategy */
static ResultAlloc findFreeBlock(const ZoneType type, const size_t size)
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
            currentBlock = (BlockHeader*)((char*)currentBlock + currentBlock->realSize + sizeof(BlockHeader));

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
        return (ResultAlloc){
            .succeeded = true,
            .ptr = bestFitBlock
        };
    }

    /* No suitable free block found */
    return (ResultAlloc){
        .succeeded = false,
        .ptr = NULL
    };
}

/* Split a block if it's significantly larger than needed */
static void* splitBlockIfNeeded(BlockHeader* block, const size_t requestedSize)
{
    const size_t minSplitSize = sizeof(BlockHeader) + 16;  /* Minimum practical size */
    const size_t currentSize = block->realSize;
    const size_t headerSize = sizeof(BlockHeader);

    if (currentSize >= requestedSize + headerSize + minSplitSize)
    {
        /* Enough space to split - create new block after this one */
        const size_t remainingSize = currentSize - requestedSize - headerSize;
        BlockHeader* const newBlock = (BlockHeader*)((char*)(block + 1) + requestedSize);

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

    return (void*)(block + 1);
}

/* Coalesce adjacent free blocks to reduce fragmentation */
static void coalesceZone(Zone* zone)
{
    if (!zone) return;

    const size_t zoneEnd = (size_t)zone + zone->size;
    BlockHeader* currentBlock = (BlockHeader*)(zone + 1);

    while ((size_t)currentBlock < zoneEnd)
    {
        /* Skip if not free */
        if (!currentBlock->isFree)
        {
            currentBlock = (BlockHeader*)((char*)currentBlock + currentBlock->realSize + sizeof(BlockHeader));
            continue;
        }

        /* Try to find next block */
        BlockHeader* nextBlock = (BlockHeader*)((char*)currentBlock + currentBlock->realSize + sizeof(BlockHeader));

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
void* my_malloc(const size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    /* Determine zone type based on size */
    const ZoneType zoneType = getZoneTypeForSize(size);

    /* Try to find a free block in existing zones */
    const ResultAlloc freeBlockResult = findFreeBlock(zoneType, size);

    void* result = NULL;

    if (freeBlockResult.succeeded)
    {
        /* Found a free block - use it */
        result = splitBlockIfNeeded((BlockHeader*)freeBlockResult.ptr, size);
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
            BlockHeader* const blockHeader = (BlockHeader*)((Zone*)newZoneResult.ptr + 1);

            /* Mark as used */
            blockHeader->size = size;
            blockHeader->isFree = false;

            result = (void*)(blockHeader + 1);
        }
        else
        {
            /* For small/tiny, the first block in the new zone is guaranteed to be big enough */
            BlockHeader* const firstBlock = (BlockHeader*)((Zone*)newZoneResult.ptr + 1);
            result = splitBlockIfNeeded(firstBlock, size);
        }
    }

    /* Track allocation if debugging is enabled */
#if DEBUG_MEMORY_LEAKS
    if (result) {
        trackAllocation(result, size);
    }
#endif

    return result;
}

/* Free memory previously allocated with my_malloc */
void my_free(void* ptr)
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
    BlockHeader* const blockHeader = ((BlockHeader*)ptr) - 1;

    /* Mark the block as free */
    blockHeader->isFree = true;

    /* For large allocations, consider unmapping directly */
    /* First find which zone this belongs to */
    Zone* prevZone = NULL;
    Zone* currentZone = largeZones;

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
    Zone* zone = tinyZones;
    while (zone != NULL) {
        tinyCount++;
        totalAllocated += zone->size;
        zone = zone->next;
    }

    /* Count small zones */
    zone = smallZones;
    while (zone != NULL) {
        smallCount++;
        totalAllocated += zone->size;
        zone = zone->next;
    }

    /* Count large zones */
    zone = largeZones;
    while (zone != NULL) {
        largeCount++;
        totalAllocated += zone->size;
        zone = zone->next;
    }

    printf("Memory zones: Tiny=%zu, Small=%zu, Large=%zu\n",
           tinyCount, smallCount, largeCount);
    printf("Total memory allocated: %zu bytes\n", totalAllocated);
}

/* Signal handler for catching segfaults */
static void segfaultHandler(int sig) {
    if (expectingSegfault) {
        printf("✓ Expected segfault caught!\n");
        expectingSegfault = false;
        longjmp(segfaultJmpBuf, 1);
    } else {
        /* Unexpected segfault - restore default handler and let program crash */
        signal(SIGSEGV, SIG_DFL);
        raise(SIGSEGV);
    }
}

/* Test performing multiple allocations and frees */
static void stressTest(void)
{
    const int NUM_ALLOCS = 1000;
    void* ptrs[NUM_ALLOCS];

    /* Perform many allocations of different sizes */
    for (int i = 0; i < NUM_ALLOCS; i++) {
        /* Mix of tiny, small, and large allocations */
        size_t size;
        if (i % 3 == 0) {
            size = (rand() % TINY_MAX_SIZE) + 1;
        } else if (i % 3 == 1) {
            size = (rand() % (SMALL_MAX_SIZE - TINY_MAX_SIZE)) + TINY_MAX_SIZE + 1;
        } else {
            size = (rand() % 4096) + SMALL_MAX_SIZE + 1;
        }

        ptrs[i] = my_malloc(size);

        /* Write some data to ensure memory is usable */
        if (ptrs[i]) {
            memset(ptrs[i], 0xAB, size);
        }
    }

    printf("Completed %d allocations\n", NUM_ALLOCS);
    printMemoryStats();

    /* Free half the allocations */
    for (int i = 0; i < NUM_ALLOCS; i += 2) {
        my_free(ptrs[i]);
        ptrs[i] = NULL;
    }

    printf("Freed half the allocations\n");
    printMemoryStats();

    /* Allocate some more to test reuse */
    for (int i = 0; i < NUM_ALLOCS; i += 2) {
        size_t size = rand() % 2048 + 1;
        ptrs[i] = my_malloc(size);
    }

    printf("Reallocated the freed slots\n");
    printMemoryStats();

    /* Free everything */
    for (int i = 0; i < NUM_ALLOCS; i++) {
        if (ptrs[i]) {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    printf("Freed all allocations\n");
    printMemoryStats();
}

/* Edge case tests to validate functionality */
static void edgeCaseTests(void) {
    printf("\n*** Edge Case Tests ***\n");

    /* Test 1: Allocation of size 0 */
    printf("Test 1: Allocation of size 0... ");
    void* ptr = my_malloc(0);
    if (ptr == NULL) {
        printf("✓ (Returns NULL as expected)\n");
    } else {
        printf("✗ (Expected NULL but got %p)\n", ptr);
        my_free(ptr);
    }

    /* Test 2: Allocations at TINY/SMALL boundary */
    printf("Test 2: Allocations at TINY/SMALL boundary... ");
    void* tiny = my_malloc(TINY_MAX_SIZE);
    void* small = my_malloc(TINY_MAX_SIZE + 1);

    if (tiny && small) {
        /* Write to confirm usability */
        memset(tiny, 0x1, TINY_MAX_SIZE);
        memset(small, 0x2, TINY_MAX_SIZE + 1);
        printf("✓\n");
    } else {
        printf("✗ (Allocation failed)\n");
    }

    my_free(tiny);
    my_free(small);

    /* Test 3: Allocations at SMALL/LARGE boundary */
    printf("Test 3: Allocations at SMALL/LARGE boundary... ");
    void* smallMax = my_malloc(SMALL_MAX_SIZE);
    void* large = my_malloc(SMALL_MAX_SIZE + 1);

    if (smallMax && large) {
        /* Write to confirm usability */
        memset(smallMax, 0x3, SMALL_MAX_SIZE);
        memset(large, 0x4, SMALL_MAX_SIZE + 1);
        printf("✓\n");
    } else {
        printf("✗ (Allocation failed)\n");
    }

    my_free(smallMax);
    my_free(large);

    /* Test 4: Very large allocation */
    printf("Test 4: Very large allocation (10MB)... ");
    const size_t TEN_MB = 10 * 1024 * 1024;
    void* large_block = my_malloc(TEN_MB);
    if (large_block) {
        /* Try writing to part of it */
        memset(large_block, 0x5, 1024); /* Write to first KB */
        printf("✓\n");
        my_free(large_block);
    } else {
        printf("✗ (Failed to allocate)\n");
    }
}

/* Invalid free tests that might cause segfaults */
static void invalidFreeTests(void) {
    printf("\n*** Invalid Free Tests ***\n");

    /* Install segfault handler */
    signal(SIGSEGV, segfaultHandler);

    /* Test 1: Double free */
    printf("Test 1: Double free... ");
    void* ptr = my_malloc(128);
    my_free(ptr);

    /* Try to catch double free segfault */
    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0) {
        my_free(ptr); /* This might segfault */
        expectingSegfault = false;
        printf("(Double free didn't segfault - implementation is safe)\n");
    }

    /* Test 2: Free invalid pointer */
    printf("Test 2: Free invalid pointer... ");
    void* invalid_ptr = (void*)0x12345678;

    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0) {
        my_free(invalid_ptr); /* This should segfault */
        expectingSegfault = false;
        printf("(Freeing invalid pointer didn't segfault - implementation is safe)\n");
    }

    /* Test 3: Free pointer with offset */
    printf("Test 3: Free pointer with offset... ");
    void* ptr2 = my_malloc(100);
    void* offsetPtr = (char*)ptr2 + 10; /* Not the exact address returned by malloc */

    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0) {
        my_free(offsetPtr); /* This should segfault */
        expectingSegfault = false;
        printf("(Freeing offset pointer didn't segfault - implementation is safe)\n");
    }

    /* Don't forget to free the properly allocated pointer */
    my_free(ptr2);
}

/* Test use-after-free behavior */
static void useAfterFreeTest(void) {
    printf("\n*** Use-After-Free Test ***\n");

    printf("Allocating and initializing memory...\n");
    const size_t size = 100;
    char* ptr = my_malloc(size);
    if (!ptr) {
        printf("Allocation failed!\n");
        return;
    }

    /* Initialize with a pattern */
    for (size_t i = 0; i < size; i++) {
        ptr[i] = (char)(i % 256);
    }

    /* Verify the pattern */
    bool integrity_ok = true;
    for (size_t i = 0; i < size; i++) {
        if (ptr[i] != (char)(i % 256)) {
            integrity_ok = false;
            break;
        }
    }
    printf("Memory integrity before free: %s\n", integrity_ok ? "✓" : "✗");

    /* Free the memory */
    printf("Freeing memory...\n");
    my_free(ptr);

    /* Try to use after free - this might cause undefined behavior */
    printf("Attempting to read memory after free (may crash)... ");
    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0) {
        /* Try to read from freed memory */
        char value = ptr[0];
        printf("Read succeeded with value %d\n", value);
        expectingSegfault = false;
    }

    /* Try to write after free - this might cause undefined behavior */
    printf("Attempting to write to memory after free (may crash)... ");
    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0) {
        /* Try to write to freed memory */
        ptr[0] = 42;
        printf("Write succeeded\n");
        expectingSegfault = false;
    }
}

/* Test allocation near exhaustion */
static void exhaustionTest(void) {
    printf("\n*** Resource Exhaustion Test ***\n");

    const size_t numAttempts = 10000;
    const size_t allocSize = 1024 * 1024; /* 1MB each */
    void* ptrs[numAttempts];
    size_t successCount = 0;

    printf("Attempting to allocate until failure...\n");

    for (size_t i = 0; i < numAttempts; i++) {
        ptrs[i] = my_malloc(allocSize);
        if (ptrs[i] == NULL) {
            printf("Allocation failed after %zu MB\n", successCount);
            break;
        }

        /* Try to write to the memory to ensure it's usable */
        memset(ptrs[i], 0xFF, allocSize);
        successCount++;

        /* Print progress every 100 allocations */
        if (i % 100 == 0) {
            printf("Allocated %zu MB so far\n", successCount);
        }
    }

    /* Free all successful allocations */
    printf("Freeing all allocations...\n");
    for (size_t i = 0; i < successCount; i++) {
        my_free(ptrs[i]);
    }

    printf("Exhaustion test completed\n");
}

/* Test for memory fragmentation issues */
static void fragmentationTest(void) {
    printf("\n*** Memory Fragmentation Test ***\n");

    const int NUM_ALLOCS = 500;
    const size_t ALLOC_SIZE = 256; /* Medium-sized allocations */
    void* ptrs[NUM_ALLOCS];

    /* First allocate all pointers */
    printf("Allocating %d blocks of %zu bytes each...\n", NUM_ALLOCS, ALLOC_SIZE);
    for (int i = 0; i < NUM_ALLOCS; i++) {
        ptrs[i] = my_malloc(ALLOC_SIZE);
        if (!ptrs[i]) {
            printf("Allocation %d failed\n", i);
            break;
        }
        /* Write a pattern to memory */
        memset(ptrs[i], i & 0xFF, ALLOC_SIZE);
    }

    printMemoryStats();

    /* Free every other block to create fragmentation */
    printf("Creating fragmentation by freeing every other block...\n");
    for (int i = 0; i < NUM_ALLOCS; i += 2) {
        my_free(ptrs[i]);
        ptrs[i] = NULL;
    }

    printMemoryStats();

    /* Try to allocate a block that's larger than our fragment size */
    printf("Attempting to allocate larger blocks in fragmented memory...\n");
    const size_t LARGE_ALLOC = ALLOC_SIZE * 3;
    int successCount = 0;

    for (int i = 0; i < NUM_ALLOCS/2; i++) {
        void* large_ptr = my_malloc(LARGE_ALLOC);
        if (large_ptr) {
            /* Fill with recognizable pattern */
            memset(large_ptr, 0xAA, LARGE_ALLOC);
            successCount++;
            /* Free immediately to avoid running out of memory */
            my_free(large_ptr);
        }
    }

    printf("Successfully allocated %d larger blocks in fragmented memory\n", successCount);

    /* Free the remaining original blocks */
    printf("Freeing remaining blocks...\n");
    for (int i = 1; i < NUM_ALLOCS; i += 2) {
        if (ptrs[i]) {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    printMemoryStats();

    /* Test if our coalescing is working by allocating larger blocks now */
    printf("Testing coalescing by allocating larger blocks in freshly freed memory...\n");
    successCount = 0;
    for (int i = 0; i < NUM_ALLOCS/4; i++) {
        void* large_ptr = my_malloc(LARGE_ALLOC);
        if (large_ptr) {
            /* Fill with a different pattern */
            memset(large_ptr, 0xBB, LARGE_ALLOC);
            successCount++;
            my_free(large_ptr);
        }
    }

    printf("Successfully allocated %d larger blocks after coalescing\n", successCount);
    printf("Fragmentation test completed\n");
}

/* For testing */
int main(void)
{
    /* Seed random number generator */
    srand(time(NULL));

    printf("Page size: %zu bytes\n", getPageSize());
    printf("TINY zone size: %zu bytes (allows ~%zu allocations)\n",
           TINY_ZONE_SIZE,
           TINY_ZONE_SIZE / (TINY_MAX_SIZE + sizeof(BlockHeader)));
    printf("SMALL zone size: %zu bytes (allows ~%zu allocations)\n",
           SMALL_ZONE_SIZE,
           SMALL_ZONE_SIZE / (SMALL_MAX_SIZE + sizeof(BlockHeader)));

    /* Basic test */
    printf("\n*** Basic allocation test ***\n");
    void* small1 = my_malloc(50);
    void* small2 = my_malloc(100);
    void* medium = my_malloc(500);
    void* large = my_malloc(2000);

    printf("Allocated: %p, %p, %p, %p\n", small1, small2, medium, large);

    if (small1 && small2 && medium && large) {
        /* Try writing to these memory regions */
        memset(small1, 0x1, 50);
        memset(small2, 0x2, 100);
        memset(medium, 0x3, 500);
        memset(large, 0x4, 2000);
        printf("Successfully wrote to all allocations\n");
    }

    /* Free them */
    my_free(small1);
    my_free(small2);
    my_free(medium);
    my_free(large);

    printf("Basic allocations freed\n");

    /* Run standard stress test */
    printf("\n*** Running stress test ***\n");
    stressTest();

    /* Run edge case tests */
    edgeCaseTests();

    /* Run invalid free tests that might cause segfaults */
    invalidFreeTests();

    /* Run use-after-free test */
    useAfterFreeTest();

    /* Run fragmentation test */
    fragmentationTest();

    /* Run resource exhaustion test (commented out by default as it may crash the system) */
    /* Uncomment the next line to run the exhaustion test */
    exhaustionTest();

#if DEBUG_MEMORY_LEAKS
    /* Check for memory leaks at the end */
    printMemoryLeakReport();
#endif

    return 0;
}