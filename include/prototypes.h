#pragma once
#include "types.h"

// syscall wrappers
AllocResult allocateMemory(const size_t size);
bool unallocateMemory(const void *const ptr, const size_t size);

// utils
const Chunk *findChunkInChunks(const Chunk *const chunkToFind, const Chunk *const chunks);
const Chunk *findPreviousChunkInChunks(const Chunk *const chunkToFind, const Chunk *const chunks);
const Chunk *findChunkBySizeInChunks(const size_t chunkSizeToFind, const Chunk *const chunks);
const Chunk *findLargerChunkInChunks(const size_t standardChunkSize, const Chunk *const chunks);
void addChunkToChunks(Chunk *const chunk, Chunk **const chunks);
bool removeChunkFromChunks(const Chunk *const chunk, const Chunk **const chunks);
void *offsetBytes(const void *const pointer, const size_t offset);
SplitChunks splitChunk(const Chunk *const chunkToSplit, const size_t sizeToCutout);

// variables
extern Global global;
