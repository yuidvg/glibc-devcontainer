#include "test.h"

int main()
{
    void *tiny = ftMalloc(10);
    memset(tiny, 0xaa, 10);
    void *tiny2 = ftMalloc(20);
    memset(tiny2, 0xbb, 20);
    void *small = ftMalloc(1000);
    memset(small, 0xcc, 1000);
    void *small2 = ftMalloc(1000);
    memset(small2, 0xdd, 1000);
    void *large = ftMalloc(1000000);
    memset(large, 0xee, 1000000);
    void *large2 = ftMalloc(1000000);
    memset(large2, 0xff, 1000000);
    printf("--- Initial state ---\n");
    show_alloc_mem();

    printf("\n--- Realloc tiny (10 -> 15) ---\n");
    tiny = ftRealloc(tiny, 15);
    // Check if realloc succeeded and potentially check content preservation
    if (tiny != NULL)
    {
        // Optionally check if the first 10 bytes are still 0xaa
        // memset(tiny + 10, 0x11, 5); // Fill the newly allocated part
        printf("Realloc tiny successful.\n");
    }
    else
    {
        printf("Realloc tiny failed.\n");
    }
    show_alloc_mem();

    printf("\n--- Realloc small (1000 -> 500) ---\n");
    small = ftRealloc(small, 500);
    if (small != NULL)
    {
        // Optionally check if the first 500 bytes are still 0xcc
        printf("Realloc small (shrink) successful.\n");
    }
    else
    {
        printf("Realloc small (shrink) failed.\n");
    }
    show_alloc_mem();

    printf("\n--- Realloc small (500 -> 2000) ---\n");
    small = ftRealloc(small, 2000);
    if (small != NULL)
    {
        // Optionally check if the first 500 bytes are still 0xcc
        // memset(small + 500, 0x22, 1500); // Fill the newly allocated part
        printf("Realloc small (grow) successful.\n");
    }
    else
    {
        printf("Realloc small (grow) failed.\n");
    }
    show_alloc_mem();

    printf("\n--- Realloc large (1M -> 500k) ---\n");
    large = ftRealloc(large, 500000);
    if (large != NULL)
    {
        // Optionally check if the first 500k bytes are still 0xee
        printf("Realloc large (shrink) successful.\n");
    }
    else
    {
        printf("Realloc large (shrink) failed.\n");
    }
    show_alloc_mem();

    printf("\n--- Realloc large (500k -> 1.5M) ---\n");
    large = ftRealloc(large, 1500000);
    if (large != NULL)
    {
        // Optionally check if the first 500k bytes are still 0xee
        // memset(large + 500000, 0x33, 1000000); // Fill the newly allocated part
        printf("Realloc large (grow) successful.\n");
    }
    else
    {
        printf("Realloc large (grow) failed.\n");
    }
    show_alloc_mem();

    printf("\n--- Realloc NULL (-> 50) ---\n");
    void *null_realloc = ftRealloc(NULL, 50);
    if (null_realloc != NULL)
    {
        memset(null_realloc, 0x44, 50);
        printf("Realloc NULL successful.\n");
    }
    else
    {
        printf("Realloc NULL failed.\n");
    }
    show_alloc_mem();

    printf("\n--- Realloc tiny2 (20 -> 0) ---\n");
    // ftRealloc with size 0 should free the memory and return NULL
    tiny2 = ftRealloc(tiny2, 0);
    if (tiny2 == NULL)
    {
        printf("Realloc tiny2 to 0 successful (pointer is NULL).\n");
    }
    else
    {
        // This case should ideally not happen if ftRealloc behaves like standard realloc
        printf("Realloc tiny2 to 0 failed (pointer is not NULL).\n");
        // We might need to free it manually if the implementation differs
        // ftFree(tiny2);
        // tiny2 = NULL;
    }
    show_alloc_mem();

    printf("\n--- State before final frees ---\n");
    show_alloc_mem();
    ftFree(small);
    ftFree(small2);
    ftFree(tiny);
    show_alloc_mem();
    ftFree(tiny2);
    ftFree(large);
    ftFree(large2);
    return (0);
}
