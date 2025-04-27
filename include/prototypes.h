#pragma once
#include "constants.h"
#include "types.h"

#define offsetBytes(pointer, offset) ((void *)(((char *)(pointer) + (offset))))
#define alignUp(reqSize) ((reqSize + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

#define chunkHeader2mem(chunkHeaderPointer) ((void *)(offsetBytes(chunkHeaderPointer, CHUNK_HEADER_SIZE)))
#define mem2chunkHeader(memPointer) ((ChunkHeader *)(offsetBytes(memPointer, -CHUNK_HEADER_SIZE)))

#define largeChunkHeader2mem(largeChunkHeaderPointer) ((offsetBytes(largeChunkHeaderPointer, LARGE_CHUNK_HEADER_SIZE)))
#define mem2largeChunkHeader(memPointer) ((LargeChunkHeader *)(offsetBytes(memPointer, -LARGE_CHUNK_HEADER_SIZE)))

#define reqsize2AllocationSize(reqsize) (MALLOC_ALIGN_MASK + CHUNK_HEADER_SIZE + (reqsize))
#define reqsize2alignedChunkSize(reqsize) (CHUNK_HEADER_SIZE + (reqsize))
#define isAligned(pointer) (((unsigned long)(pointer) & MALLOC_ALIGN_MASK) == 0)

#define distanceToNextAlignment(pointer) (size_t)((-(uintptr_t)(pointer)) & MALLOC_ALIGN_MASK)

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(const void *const ptr, const size_t size);

// utils/chunk
const ChunkHeader *findFreeChunk(const size_t payloadSize, const ChunkHeader *const chunks, const size_t zoneSize);
const ChunkHeader *findLargerFreeChunkInChunks(const size_t payloadSize, const ChunkHeader *const chunks,
                                               const size_t zoneSize);
void splitChunk(ChunkHeader *const main, const size_t goal);

// utils/largeChunk
void pushLargeChunk(LargeChunkHeader *newbie, LargeChunkHeader **const group);
bool popLargeChunk(const LargeChunkHeader *const victim, LargeChunkHeader **const group);
LargeChunkHeader *createLargeChunk(const size_t memSize);
