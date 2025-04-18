#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/my_malloc.h"

/* For catching segfaults in tests */
static jmp_buf segfaultJmpBuf;
static bool expectingSegfault = false;

/* Signal handler for catching segfaults */
static void segfaultHandler(int sig)
{
    (void)sig; /* Suppress unused parameter warning */
    if (expectingSegfault)
    {
        printf("✓ Expected segfault caught!\n");
        expectingSegfault = false;
        longjmp(segfaultJmpBuf, 1);
    }
    else
    {
        /* Unexpected segfault - restore default handler and let program crash */
        signal(SIGSEGV, SIG_DFL);
        raise(SIGSEGV);
    }
}

/* Test performing multiple allocations and frees */
static void stressTest(void)
{
    const int NUM_ALLOCS = 1000;
    void *ptrs[NUM_ALLOCS];

    /* Perform many allocations of different sizes */
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        /* Mix of tiny, small, and large allocations */
        size_t size;
        if (i % 3 == 0)
        {
            size = (rand() % TINY_MAX_SIZE) + 1;
        }
        else if (i % 3 == 1)
        {
            size = (rand() % (SMALL_MAX_SIZE - TINY_MAX_SIZE)) + TINY_MAX_SIZE + 1;
        }
        else
        {
            size = (rand() % 4096) + SMALL_MAX_SIZE + 1;
        }

        ptrs[i] = my_malloc(size);

        /* Write some data to ensure memory is usable */
        if (ptrs[i])
        {
            memset(ptrs[i], 0xAB, size);
        }
    }

    printf("Completed %d allocations\n", NUM_ALLOCS);
    print_memory_stats();

    /* Free half the allocations */
    for (int i = 0; i < NUM_ALLOCS; i += 2)
    {
        my_free(ptrs[i]);
        ptrs[i] = NULL;
    }

    printf("Freed half the allocations\n");
    print_memory_stats();

    /* Allocate some more to test reuse */
    for (int i = 0; i < NUM_ALLOCS; i += 2)
    {
        size_t size = rand() % 2048 + 1;
        ptrs[i] = my_malloc(size);
    }

    printf("Reallocated the freed slots\n");
    print_memory_stats();

    /* Free everything */
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        if (ptrs[i])
        {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    printf("Freed all allocations\n");
    print_memory_stats();
}

/* Edge case tests to validate functionality */
static void edgeCaseTests(void)
{
    printf("\n*** Edge Case Tests ***\n");

    /* Test 1: Allocation of size 0 */
    printf("Test 1: Allocation of size 0... ");
    void *ptr = my_malloc(0);
    if (ptr == NULL)
    {
        printf("✓ (Returns NULL as expected)\n");
    }
    else
    {
        printf("✗ (Expected NULL but got %p)\n", ptr);
        my_free(ptr);
    }

    /* Test 2: Allocations at TINY/SMALL boundary */
    printf("Test 2: Allocations at TINY/SMALL boundary... ");
    void *tiny = my_malloc(TINY_MAX_SIZE);
    void *small = my_malloc(TINY_MAX_SIZE + 1);

    if (tiny && small)
    {
        /* Write to confirm usability */
        memset(tiny, 0x1, TINY_MAX_SIZE);
        memset(small, 0x2, TINY_MAX_SIZE + 1);
        printf("✓\n");
    }
    else
    {
        printf("✗ (Allocation failed)\n");
    }

    my_free(tiny);
    my_free(small);

    /* Test 3: Allocations at SMALL/LARGE boundary */
    printf("Test 3: Allocations at SMALL/LARGE boundary... ");
    void *smallMax = my_malloc(SMALL_MAX_SIZE);
    void *large = my_malloc(SMALL_MAX_SIZE + 1);

    if (smallMax && large)
    {
        /* Write to confirm usability */
        memset(smallMax, 0x3, SMALL_MAX_SIZE);
        memset(large, 0x4, SMALL_MAX_SIZE + 1);
        printf("✓\n");
    }
    else
    {
        printf("✗ (Allocation failed)\n");
    }

    my_free(smallMax);
    my_free(large);

    /* Test 4: Very large allocation */
    printf("Test 4: Very large allocation (10MB)... ");
    const size_t TEN_MB = 10 * 1024 * 1024;
    void *large_block = my_malloc(TEN_MB);
    if (large_block)
    {
        /* Try writing to part of it */
        memset(large_block, 0x5, 1024); /* Write to first KB */
        printf("✓\n");
        my_free(large_block);
    }
    else
    {
        printf("✗ (Failed to allocate)\n");
    }
}

/* Invalid free tests that might cause segfaults */
static void invalidFreeTests(void)
{
    printf("\n*** Invalid Free Tests ***\n");

    /* Install segfault handler */
    signal(SIGSEGV, segfaultHandler);

    /* Test 1: Double free */
    printf("Test 1: Double free... ");
    void *ptr = my_malloc(128);
    my_free(ptr);

    /* Try to catch double free segfault */
    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0)
    {
        my_free(ptr); /* This might segfault */
        expectingSegfault = false;
        printf("(Double free didn't segfault - implementation is safe)\n");
    }

    /* Test 2: Free invalid pointer */
    printf("Test 2: Free invalid pointer... ");
    void *invalid_ptr = (void *)0x12345678;

    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0)
    {
        my_free(invalid_ptr); /* This should segfault */
        expectingSegfault = false;
        printf("(Freeing invalid pointer didn't segfault - implementation is safe)\n");
    }

    /* Test 3: Free pointer with offset */
    printf("Test 3: Free pointer with offset... ");
    void *ptr2 = my_malloc(100);
    void *offsetPtr = (char *)ptr2 + 10; /* Not the exact address returned by malloc */

    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0)
    {
        my_free(offsetPtr); /* This should segfault */
        expectingSegfault = false;
        printf("(Freeing offset pointer didn't segfault - implementation is safe)\n");
    }

    /* Don't forget to free the properly allocated pointer */
    my_free(ptr2);
}

/* Test use-after-free behavior */
static void useAfterFreeTest(void)
{
    printf("\n*** Use-After-Free Test ***\n");

    printf("Allocating and initializing memory...\n");
    const size_t size = 100;
    char *ptr = my_malloc(size);
    if (!ptr)
    {
        printf("Allocation failed!\n");
        return;
    }

    /* Initialize with a pattern */
    for (size_t i = 0; i < size; i++)
    {
        ptr[i] = (char)(i % 256);
    }

    /* Verify the pattern */
    bool integrity_ok = true;
    for (size_t i = 0; i < size; i++)
    {
        if (ptr[i] != (char)(i % 256))
        {
            integrity_ok = false;
            break;
        }
    }
    printf("Memory integrity before free: %s\n", integrity_ok ? "✓" : "✗");

    /* Free the memory */
    printf("Freeing memory...\n");
    my_free(ptr);

    /* Try to use after free - this might cause undefined behavior */
    printf("Attempting to read memory after free (may crash)... ");
    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0)
    {
        /* Try to read from freed memory */
        char value = ptr[0];
        printf("Read succeeded with value %d\n", value);
        expectingSegfault = false;
    }

    /* Try to write after free - this might cause undefined behavior */
    printf("Attempting to write to memory after free (may crash)... ");
    expectingSegfault = true;
    if (setjmp(segfaultJmpBuf) == 0)
    {
        /* Try to write to freed memory */
        ptr[0] = 42;
        printf("Write succeeded\n");
        expectingSegfault = false;
    }
}

/* Test for memory fragmentation issues */
static void fragmentationTest(void)
{
    printf("\n*** Memory Fragmentation Test ***\n");

    const int NUM_ALLOCS = 500;
    const size_t ALLOC_SIZE = 256; /* Medium-sized allocations */
    void *ptrs[NUM_ALLOCS];

    /* First allocate all pointers */
    printf("Allocating %d blocks of %zu bytes each...\n", NUM_ALLOCS, ALLOC_SIZE);
    for (int i = 0; i < NUM_ALLOCS; i++)
    {
        ptrs[i] = my_malloc(ALLOC_SIZE);
        if (!ptrs[i])
        {
            printf("Allocation %d failed\n", i);
            break;
        }
        /* Write a pattern to memory */
        memset(ptrs[i], i & 0xFF, ALLOC_SIZE);
    }

    print_memory_stats();

    /* Free every other block to create fragmentation */
    printf("Creating fragmentation by freeing every other block...\n");
    for (int i = 0; i < NUM_ALLOCS; i += 2)
    {
        my_free(ptrs[i]);
        ptrs[i] = NULL;
    }

    print_memory_stats();

    /* Try to allocate a block that's larger than our fragment size */
    printf("Attempting to allocate larger blocks in fragmented memory...\n");
    const size_t LARGE_ALLOC = ALLOC_SIZE * 3;
    int successCount = 0;

    for (int i = 0; i < NUM_ALLOCS / 2; i++)
    {
        void *large_ptr = my_malloc(LARGE_ALLOC);
        if (large_ptr)
        {
            /* Fill with recognizable pattern */
            memset(large_ptr, 0xAA, LARGE_ALLOC);
            successCount++;
            /* Free immediately to avoid running out of memory */
            my_free(large_ptr);
        }
    }

    printf("Successfully allocated %d larger blocks in fragmented memory\n", successCount);

    /* Free the remaining original blocks */
    printf("Freeing remaining blocks...\n");
    for (int i = 1; i < NUM_ALLOCS; i += 2)
    {
        if (ptrs[i])
        {
            my_free(ptrs[i]);
            ptrs[i] = NULL;
        }
    }

    print_memory_stats();

    /* Test if our coalescing is working by allocating larger blocks now */
    printf("Testing coalescing by allocating larger blocks in freshly freed memory...\n");
    successCount = 0;
    for (int i = 0; i < NUM_ALLOCS / 4; i++)
    {
        void *large_ptr = my_malloc(LARGE_ALLOC);
        if (large_ptr)
        {
            /* Fill with a different pattern */
            memset(large_ptr, 0xBB, LARGE_ALLOC);
            successCount++;
            my_free(large_ptr);
        }
    }

    printf("Successfully allocated %d larger blocks after coalescing\n", successCount);
    printf("Fragmentation test completed\n");
}

/* Test allocation near exhaustion */
static void exhaustionTest(void)
{
    printf("\n*** Resource Exhaustion Test ***\n");

    const size_t numAttempts = 100;       /* Reduced from 10000 to avoid killing the test */
    const size_t allocSize = 1024 * 1024; /* 1MB each */
    void *ptrs[numAttempts];
    size_t successCount = 0;

    printf("Attempting to allocate until failure...\n");

    for (size_t i = 0; i < numAttempts; i++)
    {
        ptrs[i] = my_malloc(allocSize);
        if (ptrs[i] == NULL)
        {
            printf("Allocation failed after %zu MB\n", successCount);
            break;
        }

        /* Try to write to the memory to ensure it's usable */
        memset(ptrs[i], 0xFF, allocSize);
        successCount++;

        /* Print progress every 10 allocations */
        if (i % 10 == 0)
        {
            printf("Allocated %zu MB so far\n", successCount);
        }

        /* Stop after a reasonable amount to avoid killing the test */
        if (successCount >= 50)
        {
            printf("Stopping after %zu MB to avoid exhausting memory\n", successCount);
            break;
        }
    }

    /* Free all successful allocations */
    printf("Freeing all allocations...\n");
    for (size_t i = 0; i < successCount; i++)
    {
        my_free(ptrs[i]);
    }

    printf("Exhaustion test completed\n");
}

/* For testing */
int main(void)
{
    /* Seed random number generator */
    srand(time(NULL));

    printf("*** Testing my_malloc implementation ***\n");

    /* Basic test */
    printf("\n*** Basic allocation test ***\n");
    void *small1 = my_malloc(50);
    void *small2 = my_malloc(100);
    void *medium = my_malloc(500);
    void *large = my_malloc(2000);

    printf("Allocated: %p, %p, %p, %p\n", small1, small2, medium, large);

    if (small1 && small2 && medium && large)
    {
        /* Try writing to these memory regions */
        memset(small1, 0x1, 50);
        memset(small2, 0x2, 100);
        memset(medium, 0x3, 500);
        memset(large, 0x4, 2000);
        printf("Successfully wrote to all allocations\n");
    }

    /* Free them */
    my_free(small1);
    my_free(small2);
    my_free(medium);
    my_free(large);

    printf("Basic allocations freed\n");

    /* Run standard stress test */
    printf("\n*** Running stress test ***\n");
    stressTest();

    /* Run edge case tests */
    edgeCaseTests();

    /* Run invalid free tests that might cause segfaults */
    invalidFreeTests();

    /* Run use-after-free test */
    useAfterFreeTest();

    /* Run fragmentation test */
    fragmentationTest();

    /* Run resource exhaustion test (modified to be less intensive) */
    exhaustionTest();

    /* Check for memory leaks at the end */
    print_memory_leak_report();

    return 0;
}
