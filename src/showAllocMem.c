#include "all.h"

static size_t print_mem_block(const void *const mem, const size_t size)
{
    ft_printf("%P - %P : %u bytes\n", (unsigned long)mem, (unsigned long)offsetBytes(mem, size), size);
    return size;
}

static size_t showZone(const char *const tag, const Zone *const zone)
{
    ChunkHeader *current = (ChunkHeader *)toFirstChunk(zone);
    ft_printf("%s : %P\n", tag, (unsigned long)current);
    size_t seenSize = 0;
    size_t total = 0;
    while (seenSize < toChunkAreaSize(zone))
    {
        if (!current->isFree)
        {
            total += print_mem_block(chunk2Mem(current), current->payloadSize);
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
    ft_printf("LARGE : %P\n", (unsigned long)current);
    while (current != NULL)
    {
        total += print_mem_block(largeChunk2Mem(current), current->payloadSize);
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
    ft_printf("Total: %S bytes\n", total);
}
