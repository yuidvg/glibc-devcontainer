#pragma once

#include "external.h"

/* Metadata structure for memory blocks - placed before actual memory */
typedef struct
{
    const size_t size;     /* Size requested by user */
    const size_t realSize; /* Actual size including header */
    const bool isFree;     /* Whether this block is free */
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
    const ZoneType type;
    const size_t size;
    struct Zone *next;
} Zone;

/* Result type for allocation functions */
typedef struct
{
    const bool succeeded;
    void *ptr;
} ResultAlloc;
