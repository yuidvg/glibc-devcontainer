#include "all.h"
static ssize_t ft_putsize_t_fd(size_t n, char *digits, int fd)
{
    ssize_t printed;

    printed = 0;
    if (n >= 10)
        printed += ft_putsize_t_fd(n / 10, digits, fd);
    printed += ft_putchar_fd(digits[n % 10], fd);
    return (printed);
}

static ssize_t ft_putPtr(const uintptr_t ptr)
{
    ssize_t printed;
    const char *const base = "0123456789ABCDEF";

    printed = 0;
    if (ptr >= 16)
        printed += ft_putPtr(ptr / 16);
    printed += ft_putchar_fd(base[ptr % 16], STDOUT_FILENO);
    return (printed);
}

static size_t print_mem_block(const void *const mem, const size_t size)
{
    ft_putPtr((uintptr_t)mem);
    ft_printf(" - ");
    ft_putPtr((uintptr_t)offsetBytes(mem, size));
    ft_printf(" : ");
    ft_putsize_t_fd(size, "0123456789", 1);
    ft_printf(" bytes\n");
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
    ft_printf("Total: ");
    ft_putsize_t_fd(total, "0123456789", 1);
    ft_printf(" bytes\n");
}
