#pragma once
#include "types.h"

size_t getPageSize(void);

/* Check if a size fits in a zone type */
ZoneType getZoneTypeForSize(const size_t size);

/* Calculate required zone size based on zone type */
size_t getZoneSizeForType(const ZoneType type);

/* Create a new memory zone using mmap */
ResultAlloc createZone(const ZoneType type, const size_t requestedSize);

/* Find a free block in existing zones using best-fit strategy */
ResultAlloc findFreeBlock(const ZoneType type, const size_t size);

/* Split a block if it's significantly larger than needed */
void *splitBlockIfNeeded(BlockHeader *block, const size_t requestedSize);

/* Coalesce adjacent free blocks to reduce fragmentation */
void coalesceZone(Zone *zone);