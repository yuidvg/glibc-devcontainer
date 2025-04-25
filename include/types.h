#pragma once

#include "external.h"

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *const allocatedMemoryAddress;
} AllocResult;

typedef struct FreeChunk
{
    size_t chunkSize; /* Size in bytes, including overhead. */

    /* used only if free. */
    struct FreeChunk *nextChunk;
} FreeChunk;

typedef struct
{
    FreeChunk *tinyFreeChunks;
    FreeChunk *smallFreeChunks;
} PreallocatedZones;

typedef struct
{
    const FreeChunk *const main;
    const FreeChunk *const rest;
} SplitChunks;
