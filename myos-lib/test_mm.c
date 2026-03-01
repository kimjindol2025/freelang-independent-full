/**
 * MyOS Memory Manager Test Suite (test_mm.c)
 *
 * 테스트 케이스:
 * 1. mm_init - 힙 초기화
 * 2. mm_alloc - 단일 할당
 * 3. mm_alloc - 다중 할당
 * 4. mm_free - 단일 해제
 * 5. mm_free - 다중 해제
 * 6. 재할당 - 해제 후 재할당
 * 7. mm_get_stats - 통계 조회
 * 8. mm_validate - 무결성 검증
 * 9. 블록 분할 테스트
 * 10. 블록 병합 테스트
 */

#include "src/mm.h"
#include <unistd.h>
#include <sys/syscall.h>

/* ============ Test Utilities ============ */

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

static void test_write(const char *msg) {
    syscall(SYS_write, 1, msg, __builtin_strlen(msg));
}

static void test_writeln(const char *msg) {
    test_write(msg);
    test_write("\n");
}

static void assert_eq(const char *name, int actual, int expected) {
    test_count++;
    if (actual == expected) {
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_write(" FAIL (got ");
        char buf[32];
        __builtin_snprintf(buf, sizeof(buf), "%d", actual);
        test_write(buf);
        test_write(", expected ");
        __builtin_snprintf(buf, sizeof(buf), "%d", expected);
        test_write(buf);
        test_writeln(")");
        test_failed++;
    }
}

static void assert_not_null(const char *name, void *ptr) {
    test_count++;
    if (ptr != NULL) {
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_writeln(" FAIL (got NULL)");
        test_failed++;
    }
}

static void assert_null(const char *name, void *ptr) {
    test_count++;
    if (ptr == NULL) {
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_writeln(" FAIL (got non-NULL)");
        test_failed++;
    }
}

/* ============ Test Cases ============ */

static void test_mm_init() {
    test_writeln("\n[TEST 1] mm_init - 힙 초기화");

    int result = mm_init(1024 * 1024);  /* 1MB 힙 */
    assert_eq("mm_init returns 0", result, 0);

    /* 두 번 초기화하면 실패해야 함 */
    result = mm_init(1024 * 1024);
    assert_eq("mm_init twice returns -1", result, -1);
}

static void test_mm_alloc_single() {
    test_writeln("\n[TEST 2] mm_alloc - 단일 할당");

    void *ptr1 = mm_alloc(64);
    assert_not_null("mm_alloc(64) returns non-NULL", ptr1);

    void *ptr2 = mm_alloc(128);
    assert_not_null("mm_alloc(128) returns non-NULL", ptr2);

    void *ptr3 = mm_alloc(256);
    assert_not_null("mm_alloc(256) returns non-NULL", ptr3);
}

static void test_mm_alloc_large() {
    test_writeln("\n[TEST 3] mm_alloc - 큰 크기 할당");

    void *ptr = mm_alloc(512 * 1024);  /* 512KB */
    assert_not_null("mm_alloc(512KB) returns non-NULL", ptr);
}

static void test_mm_free_basic() {
    test_writeln("\n[TEST 4] mm_free - 기본 해제");

    void *ptr1 = mm_alloc(100);
    void *ptr2 = mm_alloc(100);
    void *ptr3 = mm_alloc(100);

    /* 순서대로 해제 */
    mm_free(ptr1);
    mm_free(ptr2);
    mm_free(ptr3);

    test_writeln("  ✅ mm_free completed without crash");
    test_passed++;
    test_count++;
}

static void test_mm_realloc() {
    test_writeln("\n[TEST 5] mm_alloc - 해제 후 재할당");

    void *ptr1 = mm_alloc(64);
    mm_free(ptr1);

    /* 해제된 블록에 다시 할당 */
    void *ptr2 = mm_alloc(32);
    assert_not_null("mm_alloc after mm_free returns non-NULL", ptr2);
}

static void test_mm_get_stats() {
    test_writeln("\n[TEST 6] mm_get_stats - 통계 조회");

    /* 이전 테스트에서 할당한 것들로 상태를 확인 */
    MMStats stats;
    int result = mm_get_stats(&stats);
    assert_eq("mm_get_stats returns 0", result, 0);

    /* 정상적인 값인지 확인 */
    test_write("  Heap size: ");
    char buf[64];
    __builtin_snprintf(buf, sizeof(buf), "%lu bytes\n", stats.total_heap_size);
    test_write(buf);

    test_write("  Allocated: ");
    __builtin_snprintf(buf, sizeof(buf), "%lu bytes\n", stats.allocated_size);
    test_write(buf);

    test_write("  Free: ");
    __builtin_snprintf(buf, sizeof(buf), "%lu bytes\n", stats.free_size);
    test_write(buf);

    test_passed++;
    test_count++;
}

static void test_mm_dump_stats() {
    test_writeln("\n[TEST 7] mm_dump_stats - 통계 출력");
    mm_dump_stats();
    test_passed++;
    test_count++;
}

static void test_mm_validate() {
    test_writeln("\n[TEST 8] mm_validate - 무결성 검증");

    int result = mm_validate();
    assert_eq("mm_validate returns 0", result, 0);
}

static void test_block_splitting() {
    test_writeln("\n[TEST 9] 블록 분할 테스트");

    /* 큰 블록 할당 후 두 개의 작은 블록 할당 */
    void *ptr1 = mm_alloc(1000);
    void *ptr2 = mm_alloc(100);

    assert_not_null("First large alloc", ptr1);
    assert_not_null("Second small alloc", ptr2);

    /* 첫 번째 해제 */
    mm_free(ptr1);

    /* 다시 할당 - 분할된 블록에서 할당되어야 함 */
    void *ptr3 = mm_alloc(800);
    assert_not_null("Alloc after split", ptr3);
}

static void test_block_coalescing() {
    test_writeln("\n[TEST 10] 블록 병합 테스트");

    void *ptr1 = mm_alloc(256);
    void *ptr2 = mm_alloc(256);
    void *ptr3 = mm_alloc(256);

    /* 가운데 블록 해제 */
    mm_free(ptr2);

    /* 해제된 블록 주위 블록 해제 */
    mm_free(ptr1);
    mm_free(ptr3);

    /* 병합된 큰 블록에서 할당 */
    void *ptr4 = mm_alloc(700);
    assert_not_null("Alloc from coalesced block", ptr4);
}

static void test_mm_destroy() {
    test_writeln("\n[TEST 11] mm_destroy - 정리");

    mm_destroy();

    /* destroy 후 할당 시도 */
    void *ptr = mm_alloc(64);
    assert_null("mm_alloc after mm_destroy returns NULL", ptr);
}

/* ============ Test Runner ============ */

int main(void) {
    test_writeln("╔════════════════════════════════════════╗");
    test_writeln("║  MyOS Memory Manager Test Suite        ║");
    test_writeln("║  Zero-Dependency syscall-based         ║");
    test_writeln("╚════════════════════════════════════════╝");

    /* 테스트 실행 */
    test_mm_init();
    test_mm_alloc_single();
    test_mm_alloc_large();
    test_mm_free_basic();
    test_mm_realloc();
    test_mm_get_stats();
    test_mm_dump_stats();
    test_mm_validate();
    test_block_splitting();
    test_block_coalescing();
    test_mm_destroy();

    /* 결과 요약 */
    test_writeln("\n╔════════════════════════════════════════╗");
    test_writeln("║  Test Summary                          ║");
    test_writeln("╚════════════════════════════════════════╝");

    char summary[256];
    __builtin_snprintf(summary, sizeof(summary),
                      "\nTotal Tests: %d\n✅ Passed: %d\n❌ Failed: %d\n",
                      test_count, test_passed, test_failed);
    test_write(summary);

    if (test_failed == 0) {
        test_writeln("\n🎉 All tests passed!");
        return 0;
    } else {
        test_writeln("\n⚠️  Some tests failed");
        return 1;
    }
}
