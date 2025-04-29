#include "all.h"
#include "munit/munit.h"

// グローバル変数 zones を参照 (initialization.c で定義)
extern Zones zones;

// --- Test Cases ---

// initializePreAllocatedZones によって初期化された tiny zone を検証するテスト
static MunitResult test_initialization_tiny_zone(const MunitParameter params[], void *user_data_or_fixture)
{
    (void)params; // 未使用パラメータをキャストして警告を抑制
    (void)user_data_or_fixture;

    // コンストラクタが実行された結果、zones.tiny.base が NULL でないはず
    munit_assert_ptr_not_null(zones.tiny.base);

    // 期待されるサイズが設定されているか
    munit_assert_size(zones.tiny.baseSize, ==, TINY_ZONE_SIZE);

    // frontPadSize が計算されているか (具体的な値は環境依存だが、0以上であるはず)
    munit_assert_size(zones.tiny.frontPadSize, >=, 0);
    // パディング後のアドレスが MALLOC_ALIGNMENT の倍数になっているか
    const uintptr_t firstChunkAddr = (uintptr_t)zones.tiny.base + zones.tiny.frontPadSize;
    munit_assert_uintptr(firstChunkAddr % MALLOC_ALIGNMENT, ==, 0);

    // 最初のチャンクヘッダが初期化されているか
    const ChunkHeader *const firstChunk = (ChunkHeader *)firstChunkAddr;
    munit_assert_true(firstChunk->isFree);

    // 最初のチャンクのペイロードサイズが期待通りか
    const size_t expectedPayload = zones.tiny.baseSize - zones.tiny.frontPadSize - CHUNK_HEADER_SIZE;
    munit_assert_size(firstChunk->payloadSize, ==, expectedPayload);

    return MUNIT_OK; // テスト成功
}

// initializePreAllocatedZones によって初期化された small zone を検証するテスト
static MunitResult test_initialization_small_zone(const MunitParameter params[], void *user_data_or_fixture)
{
    (void)params;
    (void)user_data_or_fixture;

    // Small Zone の検証 (Tiny Zone と同様)
    munit_assert_ptr_not_null(zones.small.base);
    munit_assert_size(zones.small.baseSize, ==, SMALL_ZONE_SIZE);
    munit_assert_size(zones.small.frontPadSize, >=, 0);

    const uintptr_t firstChunkAddr = (uintptr_t)zones.small.base + zones.small.frontPadSize;
    munit_assert_uintptr(firstChunkAddr % MALLOC_ALIGNMENT, ==, 0);

    const ChunkHeader *const firstChunk = (ChunkHeader *)firstChunkAddr;
    munit_assert_true(firstChunk->isFree);

    const size_t expectedPayload = zones.small.baseSize - zones.small.frontPadSize - CHUNK_HEADER_SIZE;
    munit_assert_size(firstChunk->payloadSize, ==, expectedPayload);

    return MUNIT_OK;
}

// --- Test Suite Setup ---

// このファイル内のテストを配列にまとめる
static MunitTest initialization_tests[] = {
    {"/initialization/tiny-zone", test_initialization_tiny_zone, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {"/initialization/small-zone", test_initialization_small_zone, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    // 他の初期化関連テストがあればここに追加
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL} // 配列の終端マーカー
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
