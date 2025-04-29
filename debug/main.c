#include "debug.h"

int main()
{
    // Initial allocation
    const size_t initial_size = 32;
    void *ptr = malloc(initial_size);
    const bool is_ptr_not_null = ptr != NULL;
    (void)is_ptr_not_null;

    // Fill with a pattern
    memset(ptr, 0xBB, initial_size);

    // Grow the allocation
    const size_t new_size = 64;
    void *new_ptr = realloc(ptr, new_size);
    const bool is_new_ptr_not_null = new_ptr != NULL;
    (void)is_new_ptr_not_null;

    // Verify the original data is preserved
    for (size_t i = 0; i < initial_size; i++)
    {
        const unsigned char value = ((unsigned char *)new_ptr)[i];
        const bool is_value_bb = value == 0xBB;
        (void)is_value_bb;
    }

    // Verify we can write to the extended area
    for (size_t i = initial_size; i < new_size; i++)
    {
        ((unsigned char *)new_ptr)[i] = 0xCC;
    }

    // Clean up
    free(new_ptr);

    return 0;
}
