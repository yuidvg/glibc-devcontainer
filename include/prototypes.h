#pragma once
#include "constants.h"
#include "types.h"

#define offsetBytes(pointer, offset) ((void *)(((char *)(pointer) + (offset))))

#define chunkHeader2mem(chunkHeaderPointer) ((void *)(offsetBytes(chunkHeaderPointer, CHUNK_HEADER_SIZE)))
#define mem2chunkHeader(memPointer) ((ChunkHeader *)(offsetBytes(memPointer, -CHUNK_HEADER_SIZE)))

#define reqsize2chunksize(reqsize) (MALLOC_ALIGN_MASK + CHUNK_HEADER_SIZE + (reqsize))

#define isAligned(pointer) (((unsigned long)(pointer) & MALLOC_ALIGN_MASK) == 0)

#define rawAllocatedMemory2padSize(rawAllocatedMemoryPointer)                                                          \
    ((size_t)(((long unsigned int)(chunkHeader2mem(rawAllocatedMemoryPointer)) & MALLOC_ALIGN_MASK)))

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(const void *const ptr, const size_t size);

// utils
const ChunkHeader *findChunkInChunks(const ChunkHeader *const chunkToFind, const ChunkHeader *const chunks);
const ChunkHeader *findPreviousChunkInChunks(const ChunkHeader *const chunkToFind, const ChunkHeader *const chunks);
const ChunkHeader *findChunkBySizeInChunks(const size_t chunkSizeToFind, const ChunkHeader *const chunks);
const ChunkHeader *findLargerChunkInChunks(const size_t standardChunkSize, const ChunkHeader *const chunks);
void addChunkToChunks(ChunkHeader *const chunk, ChunkHeader **const chunks);
bool removeChunkFromChunks(const ChunkHeader *const chunk, const ChunkHeader **const chunks);
ChunkHeader *createChunk(const size_t memSize);
SplitChunks splitChunk(const ChunkHeader *const chunkToSplit, const size_t sizeToCutout);
