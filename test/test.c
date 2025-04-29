#define _POSIX_C_SOURCE 200809L // Required for popen, pclose

#include "test.h"
#include "munit.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// --- Test Functions ---

static MunitResult test_library_existence(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Test if the library file exists
    const char *cmd = "ls -l build/bin/libft_malloc*.so";
    FILE *fp = popen(cmd, "r");

    if (fp == NULL)
    {
        return MUNIT_ERROR;
    }

    // Read command output
    char buffer[1024];
    size_t output_len = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[output_len] = '\0';
    pclose(fp);

    // Check if output contains the library name
    if (strstr(buffer, "libft_malloc_") == NULL)
    {
        printf("Library not found: %s\n", buffer);
        return MUNIT_FAIL;
    }

    return MUNIT_OK;
}

static MunitResult test_symlink_existence(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Test if the symlink exists
    const char *cmd = "readlink build/bin/libft_malloc.so";
    FILE *fp = popen(cmd, "r");

    if (fp == NULL)
    {
        return MUNIT_ERROR;
    }

    // Read command output
    char buffer[1024];
    size_t output_len = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[output_len] = '\0';
    pclose(fp);

    // Check if the link points to a libft_malloc_HOSTTYPE.so file
    if (strstr(buffer, "libft_malloc_") == NULL)
    {
        printf("Symlink does not point to correct file: %s\n", buffer);
        return MUNIT_FAIL;
    }

    return MUNIT_OK;
}

static MunitResult test_exported_symbols(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Check if only the required symbols are exported
    const char *cmd = "nm -gD build/bin/libft_malloc.so | grep ' T '";
    FILE *fp = popen(cmd, "r");

    if (fp == NULL)
    {
        return MUNIT_ERROR;
    }

    // Read command output
    char buffer[4096];
    size_t output_len = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[output_len] = '\0';
    pclose(fp);

    // Check for required symbols
    const bool has_malloc = strstr(buffer, " malloc") != NULL;
    const bool has_free = strstr(buffer, " free") != NULL;
    const bool has_realloc = strstr(buffer, " realloc") != NULL;
    const bool has_show_alloc_mem = strstr(buffer, " show_alloc_mem") != NULL;

    // Check for unwanted symbols
    const bool has_printf = strstr(buffer, " printf") != NULL;
    const bool has_new = strstr(buffer, " new") != NULL;

    // All required symbols must be present
    if (!has_malloc || !has_free || !has_realloc || !has_show_alloc_mem)
    {
        printf("Missing required symbols: %s%s%s%s\n", has_malloc ? "" : "malloc ", has_free ? "" : "free ",
               has_realloc ? "" : "realloc ", has_show_alloc_mem ? "" : "show_alloc_mem ");
        return MUNIT_FAIL;
    }

    // No unwanted symbols should be present
    if (has_printf || has_new)
    {
        printf("Unwanted symbols detected: %s%s\n", has_printf ? "printf " : "", has_new ? "new " : "");
        return MUNIT_FAIL;
    }

    return MUNIT_OK;
}

static MunitResult test_basic_malloc(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Test basic malloc functionality
    const size_t test_size = 42;
    void *ptr = malloc(test_size);

    // Check if allocation succeeded
    munit_assert_not_null(ptr);

    // Check if we can write to the allocated memory
    memset(ptr, 0xAA, test_size);

    // Check if we can read from the allocated memory
    const unsigned char expected_value = 0xAA;
    for (size_t i = 0; i < test_size; i++)
    {
        const unsigned char actual_value = ((unsigned char *)ptr)[i];
        munit_assert_uint8(actual_value, ==, expected_value);
    }

    // Clean up
    free(ptr);

    return MUNIT_OK;
}

static MunitResult test_malloc_zero(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Test malloc with size 0
    void *ptr = malloc(0);

    // According to POSIX, malloc(0) can return either NULL or a unique pointer
    // that can be safely passed to free
    if (ptr != NULL)
    {
        free(ptr);
    }

    return MUNIT_OK;
}

static MunitResult test_free_null(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Test free(NULL) - should not crash
    free(NULL);

    return MUNIT_OK;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
static MunitResult test_double_free_detection(const MunitParameter params[], void *user_data)
{
    // Suppress unused parameter warnings
    (void)params;
    (void)user_data;

    // Allocate memory
    void *ptr = malloc(42);
    munit_assert_not_null(ptr);

    // Free it once (should be fine)
    free(ptr);

    // Free it again (should NOT crash - either detect and ignore, or abort)
    // We can't easily catch abort(), so we'll just call it and if it aborts,
    // the test will fail by crashing
    free(ptr);

    return MUNIT_OK;
}
#pragma GCC diagnostic pop

// --- Test Suite Definition ---

static MunitTest ft_malloc_tests[] = {{
                                          "/smoke/library-exists", test_library_existence,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {
                                          "/smoke/symlink-exists", test_symlink_existence,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {
                                          "/smoke/symbols-check", test_exported_symbols,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {
                                          "/api/malloc/basic", test_basic_malloc,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {
                                          "/api/malloc/zero-size", test_malloc_zero,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {
                                          "/api/free/null", test_free_null,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {
                                          "/api/free/double-free", test_double_free_detection,
                                          NULL, // setup
                                          NULL, // tear_down
                                          MUNIT_TEST_OPTION_NONE,
                                          NULL // parameters
                                      },
                                      {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

// --- Main Function ---

int main(int argc, char *argv[])
{
    const MunitSuite suite = {
        "/ft-malloc",           // name
        ft_malloc_tests,        // tests
        NULL,                   // suites
        1,                      // iterations
        MUNIT_SUITE_OPTION_NONE // options
    };

    return munit_suite_main(&suite, NULL, argc, argv);
}
