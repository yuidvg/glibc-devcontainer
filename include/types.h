#pragma once

#include "constants.h"
#include "external.h"

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *const allocatedMemoryAddress;
} AllocResult;



typedef struct
{
    size_t frontPadSize;
    size_t payloadSize;
    const LargeChunkHeader *next;
} LargeChunkHeader;

__attribute__((aligned(MALLOC_ALIGNMENT))) typedef struct ChunkHeader
{
    size_t payloadSize;
    bool isFree;
} ChunkHeader;
static_assert(sizeof(ChunkHeader) % MALLOC_ALIGNMENT == 0, "ChunkHeader is not aligned to MALLOC_ALIGNMENT");

typedef struct
{
    const void *base;
    size_t size;
    size_t frontPadSize;
} Zone;

typedef struct
{
    Zone tiny;
    Zone small;
    const LargeChunkHeader *large;
} Zones;

extern Zones zones;

typedef struct
{
    const ChunkHeader *const main;
    const ChunkHeader *const rest;
} SplitChunks;
