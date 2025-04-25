#pragma once

#include "external.h"

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *const allocatedMemoryAddress;
} AllocResult;

// pad
typedef struct Chunk
{
    size_t chunkSize; /* Size in bytes, including overhead. */
    size_t padSize;   /* Size of padding to align to MALLOC_ALIGNMENT. */
    bool isFree;
    struct Chunk *nextChunk;
} Chunk;
// -> mem
// (payloads)

typedef struct
{
    const Chunk *tinyFreeChunks;
    const Chunk *smallFreeChunks;
} PreallocatedZones;

typedef struct
{
    const PreallocatedZones preallocatedZones;
    allocated;
} Allocator;

typedef struct
{
    const Chunk *const main;
    const Chunk *const rest;
} SplitChunks;
