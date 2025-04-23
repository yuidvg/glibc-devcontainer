#include "all.h"

/* Check if two blocks are adjacent and can be coalesced */
static bool areBlocksAdjacent(BlockHeader *first, BlockHeader *second)
{
    const char *firstEnd = (char *)(first + 1) + first->realSize;
    return (BlockHeader *)firstEnd == second;
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

PreallocatedZones *preallocatedZones = NULL;


#include <string.h> // Required for memcpy

/* Helper function to initialize a single preallocated zone */
static Zone *initializeZone(const ZoneType type, const size_t totalSize)
{
    // Allocate memory for the zone using mmap
    void *zoneMem = mmap(NULL, totalSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (zoneMem == MAP_FAILED)
    {
        perror("mmap failed for preallocated zone");
        return NULL;
    }

    // --- Initialize Zone Header ---
    // Create header data on stack (respecting const fields)
    const Zone zoneHeader = {
        .type = type,
        .size = totalSize,
        .next = NULL};
    // Copy the header data to the beginning of the allocated memory
    ft_memcpy(zoneMem, &zoneHeader, sizeof(Zone));

    // --- Initialize First Block Header ---
    // Calculate address for the first block header
    BlockHeader *const firstBlockAddr = (BlockHeader *)((char *)zoneMem + sizeof(Zone));

    // Calculate the real size available for the first block
    const size_t headerTotalSize = sizeof(Zone) + sizeof(BlockHeader);
    const size_t blockRealSize = (totalSize > headerTotalSize) ? (totalSize - headerTotalSize) : 0;

    // Create block header data on stack (respecting const fields)
    const BlockHeader blockHeader = {
        .size = 0, // Initially unused, so requested size is 0
        .realSize = blockRealSize,
        .isFree = (blockRealSize > 0) // Mark as free only if there's actual space
    };

    // Copy the block header data into the allocated memory after the zone header
    ft_memcpy(firstBlockAddr, &blockHeader, sizeof(BlockHeader));

    return (Zone *)zoneMem;
}

__attribute__((constructor))
void initializePreAllocatedZones()
{
    // Allocate memory for the management struct itself using mmap
    preallocatedZones = mmap(NULL, sizeof(PreallocatedZones), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (preallocatedZones == MAP_FAILED)
    {
        perror("mmap failed for PreallocatedZones struct in constructor");
        preallocatedZones = NULL; // Ensure it's NULL if allocation fails
        return;
    }

    // Initialize TINY zone
    Zone *tinyZone = initializeZone(ZONE_TINY, TINY_ZONE_SIZE);
    if (!tinyZone)
    {
        // Failed to create TINY zone, clean up management struct and exit
        munmap(preallocatedZones, sizeof(PreallocatedZones));
        preallocatedZones = NULL;
        return;
    }

    // Initialize SMALL zone
    Zone *smallZone = initializeSingleZone(ZONE_SMALL, SMALL_ZONE_SIZE);
    if (!smallZone)
    {
        // Failed to create SMALL zone, clean up TINY zone and management struct
        munmap(tinyZone, TINY_ZONE_SIZE);
        munmap(preallocatedZones, sizeof(PreallocatedZones));
        preallocatedZones = NULL;
        return;
    }

    // Assign the initialized zones to the global structure
    // NOTE: Casting Zone* to Chunk*. Assumes layout compatibility or specific usage pattern.
    preallocatedZones->tiny = (Chunk *)tinyZone;
    preallocatedZones->small = (Chunk *)smallZone;
}

/* The main memory allocation function */
void *ftMalloc(const size_t size)
{
    if (size == 0)
    {
        return NULL;
    }

    /* Determine zone type based on size */
    const ZoneType zoneType = getZoneType(size);

    /* Try to find a free block in existing zones */
    const AllocResult freeBlockResult = findFreeBlock(zoneType, size);

    void *result = NULL;

    if (freeBlockResult.succeeded)
    {
        /* Found a free block - use it */
        result = splitBlockIfNeeded((BlockHeader *)freeBlockResult.ptr, size);
    }
    else
    {
        /* Need to create a new zone */
        const AllocResult newZoneResult = createZone(zoneType, size);

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

    return result;
}
