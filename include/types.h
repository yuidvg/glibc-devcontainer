#pragma once

#include "external.h"

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *const allocatedMemoryAddress;
} AllocResult;

// pad (<= MALLOC_ALIGN_MASK)
typedef struct ChunkHeader
{
    size_t chunkSize; /* Size in bytes, including overhead(header + pad + payload). */
    size_t padSize;   /* Size of padding to align to MALLOC_ALIGNMENT. */
    bool isFree;
    const struct ChunkHeader *nextChunkHeader;
} ChunkHeader;
// mem (should be aligned to MALLOC_ALIGNMENT) ->
// (payloads)

// variables
typedef struct
{
    const ChunkHeader *tinyChunks;
    const ChunkHeader *smallChunks;
    const ChunkHeader *largeChunks;
} Zones;

extern Zones zones;


typedef struct
{
    const ChunkHeader *const main;
    const ChunkHeader *const rest;
} SplitChunks;
