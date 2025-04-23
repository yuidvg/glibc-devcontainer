#pragma once

#include "external.h"

/* Metadata structure for memory blocks - placed before actual memory */
typedef struct
{
    const size_t size;     /* Size requested by user */
    const size_t realSize; /* Actual size including header */
    const bool isFree;     /* Whether this block is free */
} BlockHeader;

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *const allocatedMemoryAddress;
} AllocResult;

typedef struct
{

    size_t mchunk_prev_size; /* Size of previous chunk (if free).  */
    size_t mchunk_size;      /* Size in bytes, including overhead. */

    struct chunk *fd; /* double links -- used only if free. */
    struct chunk *bk;

    /* Only used for large blocks: pointer to next larger size.  */
    struct chunk *fd_nextsize; /* double links -- used only if free. */
    struct chunk *bk_nextsize;
} Chunk;

typedef struct
{
    Chunk *tiny;
    Chunk *small;
} PreallocatedZones;
