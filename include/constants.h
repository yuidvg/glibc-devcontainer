#pragma once

#include "external.h"

/* Size of allocation zones - tuned for at least 100 allocations per zone */
#define TINY_ZONE_SIZE (sysconf(_SC_PAGESIZE) * 16)   /* N: Size for tiny zones */
#define SMALL_ZONE_SIZE (sysconf(_SC_PAGESIZE) * 128) /* M: Size for small zones */

#define TINY_MAX_SIZE 128   /* n: max size for tiny allocations */
#define SMALL_MAX_SIZE 1024 /* m: max size for small allocations */

#define CHUNK_HEADER_SIZE sizeof(size_t)

#define MALLOC_ALIGNMENT sizeof(size_t) * 2
#define MALLOC_ALIGN_MASK (MALLOC_ALIGNMENT - 1)

#define chunk2mem(p) ((void *)((char *)(p) + CHUNK_HEADER_SIZE))

#define memsize2chunksize(memsize)                                                                                     \
    (((memsize) + CHUNK_HEADER_SIZE + MALLOC_ALIGN_MASK < MINSIZE)                                                     \
         ? MINSIZE                                                                                                     \
         : ((memsize) + CHUNK_HEADER_SIZE + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

#define isAligned(m) (((unsigned long)(m) & MALLOC_ALIGN_MASK) == 0)
