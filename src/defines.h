#pragma once
#include "external.h"

#define TINY_ZONE_SIZE (sysconf(_SC_PAGESIZE) * 128) /* N: Size for tiny zones */
/* Size of allocation zones - tuned for at least 100 allocations per zone */
#define SMALL_ZONE_SIZE (sysconf(_SC_PAGESIZE) * 256) /* M: Size for small zones */

#define TINY_MAX_SIZE 128   /* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024 /* m: max size for small allocations */

#define MALLOC_ALIGNMENT sizeof(size_t) * 2
#define MALLOC_ALIGN_MASK (MALLOC_ALIGNMENT - 1)

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *const allocatedMemoryAddress;
} AllocResult;

typedef struct LargeChunkHeader
{
    size_t frontPadSize;
    size_t payloadSize;
    struct LargeChunkHeader *next;
} LargeChunkHeader;

__attribute__((aligned(MALLOC_ALIGNMENT))) typedef struct
{
    size_t payloadSize;
    bool isFree;
} ChunkHeader;
static_assert(sizeof(ChunkHeader) % MALLOC_ALIGNMENT == 0, "ChunkHeader is not aligned to MALLOC_ALIGNMENT");

#define LARGE_CHUNK_HEADER_SIZE sizeof(LargeChunkHeader)
#define CHUNK_HEADER_SIZE sizeof(ChunkHeader)

#define PAYLOAD_MINIMUM_SIZE alignUp(1)
#define CHUNK_MINIMUM_SIZE (PAYLOAD_MINIMUM_SIZE + CHUNK_HEADER_SIZE)

typedef struct
{
    const void *base;
    size_t baseSize;
    size_t frontPadSize;
} Zone;

typedef struct
{
    Zone tiny;
    Zone small;
    LargeChunkHeader *large;
} Zones;

extern Zones zones;

typedef struct
{
    const ChunkHeader *const main;
    const ChunkHeader *const rest;
} SplitChunks;
