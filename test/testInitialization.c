#include "test.h"
#include "munit.h"

// --- Test Cases ---

// 初期化後のメモリ割り当てが機能することを確認するテスト
static MunitResult test_initialization_tiny_allocations(const MunitParameter params[], void *user_data_or_fixture)
{
    (void)params; // 未使用パラメータをキャストして警告を抑制
    (void)user_data_or_fixture;

    // 複数の小さな割り当てが成功することを確認
    void *ptrs[100];
    const size_t small_size = 64; // TINY_MAX_SIZEより小さい値

    // 小さなサイズの割り当てを複数回行い、すべて成功することを確認
    for (int i = 0; i < 100; i++) {
        ptrs[i] = malloc(small_size);
        munit_assert_ptr_not_null(ptrs[i]);

        // 割り当てられたメモリが使用可能であることを確認
        // (アドレスが適切にアラインされていることを間接的に確認)
        memset(ptrs[i], 0xAA, small_size);
    }

    // 割り当てたメモリをクリーンアップ
    for (int i = 0; i < 100; i++) {
        free(ptrs[i]);
    }

    return MUNIT_OK; // テスト成功
}

// 初期化後により大きなメモリ割り当てが機能することを確認するテスト
static MunitResult test_initialization_small_allocations(const MunitParameter params[], void *user_data_or_fixture)
{
    (void)params;
    (void)user_data_or_fixture;

    // 複数の中サイズの割り当てが成功することを確認
    void *ptrs[20];
    const size_t medium_size = 512; // TINY_MAX_SIZEより大きく、SMALL_MAX_SIZEより小さい値

    // 中サイズの割り当てを複数回行い、すべて成功することを確認
    for (int i = 0; i < 20; i++) {
        ptrs[i] = malloc(medium_size);
        munit_assert_ptr_not_null(ptrs[i]);

        // 割り当てられたメモリが使用可能であることを確認
        memset(ptrs[i], 0xBB, medium_size);
    }

    // 割り当てたメモリをクリーンアップ
    for (int i = 0; i < 20; i++) {
        free(ptrs[i]);
    }

    return MUNIT_OK;
}

// 初期化後に大きなメモリ割り当てが機能することを確認するテスト
static MunitResult test_initialization_large_allocations(const MunitParameter params[], void *user_data_or_fixture)
{
    (void)params;
    (void)user_data_or_fixture;

    // 大きなサイズの割り当てが成功することを確認
    const size_t large_size = 2048; // SMALL_MAX_SIZEより大きい値

    void *ptr = malloc(large_size);
    munit_assert_ptr_not_null(ptr);

    // 割り当てられたメモリが使用可能であることを確認
    memset(ptr, 0xCC, large_size);

    // クリーンアップ
    free(ptr);

    return MUNIT_OK;
}

// --- Test Suite Setup ---

// このファイル内のテストを配列にまとめる
static MunitTest initialization_tests[] = {
    {"/initialization/tiny-allocations", test_initialization_tiny_allocations, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/initialization/small-allocations", test_initialization_small_allocations, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/initialization/large-allocations", test_initialization_large_allocations, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL} // End of array
};

// テストスイートを定義
static const MunitSuite initialization_suite = {
    "/initialization",      // スイートの名前 (スラッシュで始まる)
    initialization_tests,   // このスイートに含まれるテストの配列
    NULL,                   // ネストされたスイート (今回はなし)
    1,                      // 繰り返し回数
    MUNIT_SUITE_OPTION_NONE // オプション
};

// --- Main Function ---

// µnit のテストを実行するための main 関数
int main(int argc, char *argv[])
{
    return munit_suite_main(&initialization_suite, NULL, argc, argv);
}
