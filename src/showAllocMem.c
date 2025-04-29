#include "all.h"

static size_t showZone(const char *const tag, const Zone *const zone)
{
    ChunkHeader *current = (ChunkHeader *)toFirstChunk(zone);
    printf("%s : 0x%lX\n", tag, (unsigned long)current);
    size_t seenSize = 0;
    size_t total = 0;
    while (seenSize < toChunkAreaSize(zone))
    {
        if (!current->isFree)
        {
            printf("0x%lX - 0x%lX : %zu bytes\n", (unsigned long)chunk2Mem(current),
                   (unsigned long)offsetBytes(chunk2Mem(current), current->payloadSize), current->payloadSize);
            total += current->payloadSize;
        }
        seenSize += toChunkSize(current);
        current = toNext(current);
    }
    return total;
}

static size_t showLargeZone()
{
    LargeChunkHeader *current = zones.large;
    size_t total = 0;
    printf("LARGE : 0x%lX\n", (unsigned long)current);
    while (current != NULL)
    {
        printf("0x%lX - 0x%lX : %zu bytes\n", (unsigned long)largeChunk2Mem(current),
               (unsigned long)offsetBytes(largeChunk2Mem(current), current->payloadSize), current->payloadSize);
        total += current->payloadSize;
        current = current->next;
    }
    return total;
}

void show_allocated_memories()
{
    size_t total = 0;
    total += showZone("TINY", &zones.tiny);
    total += showZone("SMALL", &zones.small);
    total += showLargeZone();
    printf("Total: %zu bytes\n", total);
}
