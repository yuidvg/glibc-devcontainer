#pragma once
#include "defines.h"

#define offsetBytes(pointer, offset) ((void *)(((char *)(pointer) + (offset))))
#define alignUp(reqSize) ((reqSize + MALLOC_ALIGN_MASK) & ~MALLOC_ALIGN_MASK)

#define chunkHeader2mem(chunkHeaderPointer) ((void *)(offsetBytes(chunkHeaderPointer, sizeof(ChunkHeader))))
#define mem2chunkHeader(memPointer) ((ChunkHeader *)(offsetBytes(memPointer, -sizeof(ChunkHeader))))

#define mem2largeChunkHeader(memPointer) ((LargeChunkHeader *)(offsetBytes(memPointer, -sizeof(LargeChunkHeader))))

#define reqsize2AllocationSize(reqsize) (MALLOC_ALIGN_MASK + sizeof(LargeChunkHeader) + (reqsize))
#define reqsize2alignedChunkSize(reqsize) (sizeof(ChunkHeader) + (reqsize))
#define isAligned(pointer) (((unsigned long)(pointer) & MALLOC_ALIGN_MASK) == 0)

#define distanceToNextAlignment(pointer) (size_t)((-(uintptr_t)(pointer)) & MALLOC_ALIGN_MASK)

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(const void *const ptr, const size_t size);

// utils/zone
size_t toChunkAreaSize(const Zone *const zone);
ChunkHeader *toFirstChunk(const Zone *const zone);
void *toZoneEnd(const Zone *const zone);

// utils/chunk
Zone *toZone(const void *const mem);
void *chunk2Mem(const ChunkHeader *const chunk);
ChunkHeader *toNext(const ChunkHeader *const original);
size_t toChunkSize(const ChunkHeader *const chunk);
ChunkHeader *findChunk(const void *const mem, const Zone *const zone);
const ChunkHeader *findFreeChunk(const size_t payloadSize, const Zone *const zone);
ChunkHeader *findFittableFreeChunk(const size_t payloadSize, const Zone *const zone);
ChunkHeader *findFree(ChunkHeader *const start, const void *const end);
void splitChunk(ChunkHeader *const main, const size_t goalPayload);
size_t consequtiveFreeChunksSize(const ChunkHeader *const start);
bool expand(ChunkHeader *const expandee, const size_t by);
bool expandChunk(ChunkHeader *const expandee, const size_t minGoalPayloadSize);

// utils/largeChunk
void *largeChunk2Mem(const LargeChunkHeader *const header);
void *toBase(const LargeChunkHeader *const header);
size_t toBaseSize(const LargeChunkHeader *const header);
LargeChunkHeader *findLargeChunk(const void *const mem);
LargeChunkHeader *toPrevious(const LargeChunkHeader *const original);
void push(LargeChunkHeader *newbie);
bool pop(const LargeChunkHeader *const victim);
LargeChunkHeader *createLargeChunk(const size_t memSize);

// error
void printError(const char *const message);

// final functions
void *ftMalloc(const size_t reqSize);
void ftFree(const void *const mem);
void *ftRealloc(const void *const originalMem, const size_t reqSize);
void show_allocated_memories();