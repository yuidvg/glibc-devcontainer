#pragma once
#include "types.h"

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(const void *const ptr, const size_t size);

// utils
const FreeChunk *findChunkInChunks(const FreeChunk *const chunkToFind, const FreeChunk *const *const chunks);
const FreeChunk *findPreviousChunkInChunks(const FreeChunk *const chunkToFind, const FreeChunk *const *const chunks);
const FreeChunk *findChunkBySizeInChunks(const size_t chunkSizeToFind, const FreeChunk *const *const chunks);
const FreeChunk *findLargerChunkInChunks(const size_t standardChunkSize, const FreeChunk *const *const chunks);
void addChunkToChunks(FreeChunk *const chunk, FreeChunk **const chunks);
bool removeChunkFromChunks(const FreeChunk *const chunk, const FreeChunk **const chunks);
SplitChunks splitChunk(const FreeChunk *const chunkToSplit, const size_t sizeToCutout);

// variables
extern PreallocatedZones preallocatedZones;
