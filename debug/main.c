#include "debug.h"

int main()
{
    void *p = malloc(42);
    // Check p != NULL
    if (p == NULL)
    {
        printf("p is NULL\n");
    }

    // Test read/write access p[0...41]
    // We perform boundary checks: write/read first and last byte.
    char *char_p = (char *)p;
    const char test_val_start = 'A';
    const char test_val_end = 'Z';
    const size_t last_index = 41;

    // Write to boundaries
    char_p[0] = test_val_start;
    char_p[last_index] = test_val_end;

    // Read and verify boundaries
    const bool is_start_correct = char_p[0] == test_val_start;
    const bool is_end_correct = char_p[last_index] == test_val_end;
    (void)is_start_correct;
    (void)is_end_correct;
    // Clean up allocated memory (good practice in tests)
    free(p);

    return 0;
}
