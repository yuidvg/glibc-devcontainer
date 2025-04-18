#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

#include "my_malloc.h"

/* Ensure MAP_ANONYMOUS is defined - fix platform differences */
#ifndef MAP_ANONYMOUS
#ifdef __APPLE__
#define MAP_ANONYMOUS MAP_ANON
#else
#define MAP_ANONYMOUS 0x20 /* Value on Linux systems */
#endif
#endif

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

/* Result type for allocation functions */
typedef struct
{
    bool succeeded;
    void *ptr;
} ResultAlloc;

/* Get system page size (immutable) */
size_t getPageSize(void);

/* Check if a size fits in a zone type */
ZoneType getZoneTypeForSize(size_t size);

/* Calculate required zone size based on zone type */
size_t getZoneSizeForType(ZoneType type);

/* Create a new memory zone using mmap */
ResultAlloc createZone(ZoneType type, size_t requestedSize);

/* Find a free block in existing zones using best-fit strategy */
ResultAlloc findFreeBlock(ZoneType type, size_t size);

/* Split a block if it's significantly larger than needed */
void *splitBlockIfNeeded(BlockHeader *block, size_t requestedSize);

/* Coalesce adjacent free blocks to reduce fragmentation */
void coalesceZone(Zone *zone);

#if DEBUG_MEMORY_LEAKS
/* Memory leak tracking functions */
void trackAllocation(void *ptr, size_t size);
void untrackAllocation(void *ptr);
#endif
