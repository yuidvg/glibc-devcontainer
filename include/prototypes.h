#pragma once
#include "defines.h"

#define offsetBytes(pointer, offset) ((void *)(((char *)(pointer) + (offset))))
#define alignUp(reqSize) ((reqSize + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

#define chunkHeader2mem(chunkHeaderPointer) ((void *)(offsetBytes(chunkHeaderPointer, sizeof(ChunkHeader))))
#define mem2chunkHeader(memPointer) ((ChunkHeader *)(offsetBytes(memPointer, -sizeof(ChunkHeader))))

#define mem2largeChunkHeader(memPointer) ((LargeChunkHeader *)(offsetBytes(memPointer, -sizeof(LargeChunkHeader))))

#define reqsize2AllocationSize(reqsize) (MALLOC_ALIGN_MASK + sizeof(ChunkHeader) + (reqsize))
#define reqsize2alignedChunkSize(reqsize) (sizeof(ChunkHeader) + (reqsize))
#define isAligned(pointer) (((unsigned long)(pointer) & MALLOC_ALIGN_MASK) == 0)

#define distanceToNextAlignment(pointer) (size_t)((-(uintptr_t)(pointer)) & MALLOC_ALIGN_MASK)

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(const void *const ptr, const size_t size);

// utils/zone
size_t toChunkAreaSize(const Zone *const zone);
ChunkHeader *toFirstChunk(const Zone *const zone);

// utils/chunk
void *chunk2Mem(const ChunkHeader *const chunk);
ChunkHeader *toNext(const ChunkHeader *const original);
size_t chunk2ChunkSize(const ChunkHeader *const chunk);
ChunkHeader *findChunk(const void *const mem, const Zone *const zone);
const ChunkHeader *findFreeChunk(const size_t payloadSize, const Zone *const zone);
ChunkHeader *findFittableFreeChunk(const size_t payloadSize, const Zone *const zone);
void splitChunk(ChunkHeader *const main, const size_t goal);

// utils/largeChunk
void *largeChunk2Mem(const LargeChunkHeader *const header);
void *toBase(const LargeChunkHeader *const header);
size_t toBaseSize(const LargeChunkHeader *const header);
const LargeChunkHeader *findLargeChunk(const void *const mem);
LargeChunkHeader *toPrevious(const LargeChunkHeader *const original);
void push(LargeChunkHeader *newbie);
bool pop(const LargeChunkHeader *const victim);
LargeChunkHeader *createLargeChunk(const size_t memSize);
