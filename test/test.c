#define _POSIX_C_SOURCE 200809L // Required for popen, pclose

#include "test.h"
#include "munit.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h> // For max_align_t
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// --- Test Functions ---
// #    category    title               action              expected result
// #1   malloc      basic-allocation    `p = malloc(42)`    `p != NULL` ∧ read/write `p[0…41]` OK
static MunitResult basic_allocation(const MunitParameter params[], void *user_data_or_fixture)
{
    (void)params;
    (void)user_data_or_fixture;
    void *p = malloc(42);
    // Check p != NULL
    munit_assert_ptr_not_null(p);

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
    munit_assert_char(char_p[0], ==, test_val_start);
    munit_assert_char(char_p[last_index], ==, test_val_end);

    // Clean up allocated memory (good practice in tests)
    free(p);

    return MUNIT_OK;
}

// --- Main Function ---

int main(int argc, char *argv[])
{
    MunitTest malloc_tests[] = {{"/#1 basic-allocation", basic_allocation, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
                                {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};
    MunitSuite ft_malloc_suites[] = {{"/malloc", malloc_tests, NULL, 1, MUNIT_SUITE_OPTION_NONE},
                                     {NULL, NULL, NULL, 0, MUNIT_SUITE_OPTION_NONE}};
    const MunitSuite main_suite = {
        "/ft-malloc",           // name
        NULL,                   // tests
        ft_malloc_suites,       // suites
        1,                      // iterations
        MUNIT_SUITE_OPTION_NONE // options
    };

    return munit_suite_main(&main_suite, NULL, argc, argv);
}
