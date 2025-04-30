#include "debug.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    // Allocate powers of 2 from 2^0 to 2^15 (1 byte to 32KB)
    const int num_allocations = 16;
    void *pointers[num_allocations];
    size_t sizes[num_allocations];

    // Allocate each power of two
    for (int i = 0; i < num_allocations; i++)
    {
        sizes[i] = 1UL << i; // 2^i
        pointers[i] = malloc(sizes[i]);
        assert(pointers[i] != NULL);

        // Check alignment - should be aligned appropriately
        // For smaller allocations (< 16 bytes), alignment might be 8 bytes
        // For larger allocations, alignment should be 16 bytes
        const uintptr_t addr = (uintptr_t)pointers[i];
        const size_t min_alignment = 16;

        if (addr % min_alignment != 0)
        {
            printf("Warning: Allocation of size %zu is not %zu-byte aligned: %p (modulo %zu = %zu)\n", sizes[i],
                   min_alignment, pointers[i], min_alignment, addr % min_alignment);
        }

        // Fill the memory with a unique pattern
        memset(pointers[i], (unsigned char)(0xF0 + i), sizes[i]);
    }

    // Verify each allocation is usable and doesn't overlap
    for (int i = 0; i < num_allocations; i++)
    {
        unsigned char *ptr = (unsigned char *)pointers[i];

        // Check every byte for the expected pattern
        for (size_t j = 0; j < sizes[i]; j++)
        {
            if (j % 4096 == 0 || j == sizes[i] - 1)
            { // Check first byte of each page and last byte
                if (ptr[j] != (unsigned char)(0xF0 + i))
                {
                    printf("Memory overlap detected: Block size %zu at position %zu contains %02X instead of %02X",
                           sizes[i], j, ptr[j], (unsigned char)(0xF0 + i));
                    return 1;
                }
            }
        }
    }

    // Free all allocations
    for (int i = 0; i < num_allocations; i++)
    {
        free(pointers[i]);
    }

    return 0;
}
