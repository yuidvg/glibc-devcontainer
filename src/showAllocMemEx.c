#include "all.h"

// Build a lookup table of free chunks for quick membership testing
FreeChunksLookup buildFreeChunksLookup(const Chunk *const freeChunks)
{
	FreeChunksLookup lookup = { .count = 0 };

	const Chunk *current = freeChunks;
	while (current != NULL && lookup.count < 1024)
	{
		lookup.chunks[lookup.count++] = current;
		current = current->nextChunk;
	}

	return lookup;
}

// Check if a chunk is in the free list using the lookup table
bool isChunkFree(const void *const chunk, const FreeChunksLookup *const lookup)
{
	for (size_t i = 0; i < lookup->count; i++)
	{
		if (lookup->chunks[i] == chunk)
		{
			return true;
		}
	}
	return false;
}

// Display all allocated chunks in a memory zone
size_t displayAllocatedZone(
	const void *const zoneStartAddr,
	const size_t zoneSize,
	const FreeChunksLookup *const freeLookup,
	const size_t maxChunkSize)
{
	const uint8_t *const zoneStart = (const uint8_t*)zoneStartAddr;
	const uint8_t *const zoneEnd = zoneStart + zoneSize;
	const uint8_t *currentPos = zoneStart;
	size_t totalAllocated = 0;

	while (currentPos < zoneEnd)
	{
		// Get chunk header
		const size_t chunkSize = *(const size_t*)currentPos;
		if (chunkSize == 0 || chunkSize > maxChunkSize)
		{
			// Invalid chunk size - we've reached uninitialized memory
			break;
		}

		// If not free, it's allocated - display it
		if (!isChunkFree(currentPos, freeLookup))
		{
			const size_t allocatedSize = chunkSize - CHUNK_HEADER_SIZE;
			const void *dataStart = currentPos + CHUNK_HEADER_SIZE;
			const void *dataEnd = (const uint8_t*)dataStart + allocatedSize - 1;

			printf("%p - %p : %zu bytes\n", dataStart, dataEnd, allocatedSize);
			totalAllocated += allocatedSize;
		}

		// Move to next chunk
		currentPos += chunkSize;

		// Safety check - don't go past the zone
		if (currentPos >= zoneEnd)
		{
			break;
		}
	}

	return totalAllocated;
}

void show_alloc_mem()
{
	size_t totalBytes = 0;

	// Show TINY zone
	printf("TINY : %p\n", (void*)preallocatedZones.tinyFreeChunks);

	if (preallocatedZones.tinyFreeChunks != NULL)
	{
		const FreeChunksLookup tinyFreeLookup = buildFreeChunksLookup(preallocatedZones.tinyFreeChunks);
		totalBytes += displayAllocatedZone(
			preallocatedZones.tinyFreeChunks,
			TINY_ZONE_SIZE,
			&tinyFreeLookup,
			TINY_ZONE_SIZE
		);
	}

	// Show SMALL zone
	printf("SMALL : %p\n", (void*)preallocatedZones.smallFreeChunks);

	if (preallocatedZones.smallFreeChunks != NULL)
	{
		const FreeChunksLookup smallFreeLookup = buildFreeChunksLookup(preallocatedZones.smallFreeChunks);
		totalBytes += displayAllocatedZone(
			preallocatedZones.smallFreeChunks,
			SMALL_ZONE_SIZE,
			&smallFreeLookup,
			SMALL_ZONE_SIZE
		);
	}

	// Show LARGE zone - these are directly mmapped and not in our zones
	printf("LARGE : %p\n", (void*)0); // There's no single start address for large allocations

	// Since our implementation doesn't keep a list of large allocations,
	// we can't easily print them here.
	// A proper implementation would need to maintain a global list of large allocations.

	printf("Total : %zu bytes\n", totalBytes);
}
