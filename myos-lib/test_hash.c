/**
 * MyOS HashMap Test Suite (test_hash.c)
 *
 * 테스트 케이스:
 * 1. hash_new - 해시 테이블 생성
 * 2. hash_set - 키-값 저장
 * 3. hash_get - 값 조회
 * 4. hash_delete - 엔트리 삭제
 * 5. hash_contains - 키 존재 확인
 * 6. 자동 리사이징
 * 7. 충돌 처리 (Open Addressing)
 * 8. String 키
 * 9. Struct 값
 * 10. 대량 저장
 */

#include "src/hash.h"
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

static void assert_eq_int(const char *name, int actual, int expected) {
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

static void assert_size_t(const char *name, size_t actual, size_t expected) {
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
        __builtin_snprintf(buf, sizeof(buf), "%lu", actual);
        test_write(buf);
        test_write(", expected ");
        __builtin_snprintf(buf, sizeof(buf), "%lu", expected);
        test_write(buf);
        test_writeln(")");
        test_failed++;
    }
}

typedef struct {
    int x;
    int y;
} Point;

/* ============ Test Cases ============ */

static void test_hash_new() {
    test_writeln("\n[TEST 1] hash_new - 해시 테이블 생성");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);
    assert_not_null("hash_new returns non-NULL", map);
    assert_size_t("initial size is 0", hash_size(map), 0);
    assert_eq_int("is_empty returns 1", hash_is_empty(map), 1);

    hash_free(map);
}

static void test_hash_set_get() {
    test_writeln("\n[TEST 2] hash_set & hash_get - 저장 및 조회");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    int key = 42;
    int value = 100;

    int result = hash_set(map, &key, &value);
    assert_eq_int("hash_set returns 0", result, 0);
    assert_size_t("size is 1", hash_size(map), 1);

    int *found = (int*)hash_get(map, &key);
    assert_not_null("hash_get returns non-NULL", found);
    assert_eq_int("retrieved value is 100", *found, 100);

    hash_free(map);
}

static void test_hash_update() {
    test_writeln("\n[TEST 3] hash_set - 값 업데이트");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    int key = 10;
    int value1 = 100;
    int value2 = 200;

    hash_set(map, &key, &value1);
    int *result1 = (int*)hash_get(map, &key);
    assert_eq_int("initial value is 100", *result1, 100);

    hash_set(map, &key, &value2);
    int *result2 = (int*)hash_get(map, &key);
    assert_eq_int("updated value is 200", *result2, 200);
    assert_size_t("size still 1", hash_size(map), 1);

    hash_free(map);
}

static void test_hash_delete() {
    test_writeln("\n[TEST 4] hash_delete - 엔트리 삭제");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    int key1 = 10;
    int key2 = 20;
    int value1 = 100;
    int value2 = 200;

    hash_set(map, &key1, &value1);
    hash_set(map, &key2, &value2);

    assert_size_t("size is 2", hash_size(map), 2);

    int out_value;
    int result = hash_delete(map, &key1, &out_value);
    assert_eq_int("delete returns 0", result, 0);
    assert_eq_int("deleted value is 100", out_value, 100);
    assert_size_t("size is 1", hash_size(map), 1);

    /* key1은 없어야 함 */
    int *found = (int*)hash_get(map, &key1);
    assert_null("deleted key returns NULL", found);

    /* key2는 여전히 있어야 함 */
    int *found2 = (int*)hash_get(map, &key2);
    assert_not_null("other key still exists", found2);
    assert_eq_int("other value is 200", *found2, 200);

    hash_free(map);
}

static void test_hash_contains() {
    test_writeln("\n[TEST 5] hash_contains - 키 존재 확인");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    int key1 = 10;
    int key2 = 20;
    int value = 100;

    hash_set(map, &key1, &value);

    assert_eq_int("key1 exists", hash_contains(map, &key1), 1);
    assert_eq_int("key2 not exists", hash_contains(map, &key2), 0);

    hash_free(map);
}

static void test_hash_clear() {
    test_writeln("\n[TEST 6] hash_clear - 전체 제거");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    for (int i = 0; i < 10; i++) {
        int value = i * 10;
        hash_set(map, &i, &value);
    }

    assert_size_t("size is 10", hash_size(map), 10);

    hash_clear(map);
    assert_size_t("size is 0 after clear", hash_size(map), 0);
    assert_eq_int("is_empty returns 1", hash_is_empty(map), 1);

    hash_free(map);
}

static void test_hash_autogrow() {
    test_writeln("\n[TEST 7] 자동 리사이징 테스트");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    size_t initial_capacity = hash_capacity(map);

    /* 용량을 초과할 때까지 저장 */
    for (int i = 0; i < 50; i++) {
        int value = i * 10;
        hash_set(map, &i, &value);
    }

    assert_size_t("size is 50", hash_size(map), 50);

    /* 용량이 자동으로 증가했는지 확인 */
    size_t final_capacity = hash_capacity(map);
    assert_eq_int("capacity grew", (final_capacity > initial_capacity) ? 1 : 0, 1);

    /* 모든 키 검증 */
    int *first = (int*)hash_get(map, &(int){0});
    int *last = (int*)hash_get(map, &(int){49});
    assert_eq_int("first value is 0", *first, 0);
    assert_eq_int("last value is 490", *last, 490);

    hash_free(map);
}

static void test_hash_collisions() {
    test_writeln("\n[TEST 8] 충돌 처리 (Open Addressing)");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    /* 의도적으로 충돌 생성 (같은 해시 값) */
    int keys[] = {1, 17, 33, 49, 65};  /* 모두 1 % 16 = 1 */
    int values[] = {10, 20, 30, 40, 50};

    for (int i = 0; i < 5; i++) {
        hash_set(map, &keys[i], &values[i]);
    }

    assert_size_t("size is 5", hash_size(map), 5);

    /* 모든 키 검증 */
    for (int i = 0; i < 5; i++) {
        int *val = (int*)hash_get(map, &keys[i]);
        assert_not_null("collision key found", val);
        assert_eq_int("collision value correct",
                     *val, values[i]);
    }

    hash_free(map);
}

static void test_hash_string_keys() {
    test_writeln("\n[TEST 9] 문자열 키");

    HashMap *map = hash_new(sizeof(char*), sizeof(int),
                           hash_string, cmp_string);

    char *key1 = "hello";
    char *key2 = "world";
    int value1 = 100;
    int value2 = 200;

    hash_set(map, &key1, &value1);
    hash_set(map, &key2, &value2);

    assert_size_t("size is 2", hash_size(map), 2);

    int *found1 = (int*)hash_get(map, &key1);
    int *found2 = (int*)hash_get(map, &key2);

    assert_eq_int("value1 is 100", *found1, 100);
    assert_eq_int("value2 is 200", *found2, 200);

    hash_free(map);
}

static void test_hash_struct_values() {
    test_writeln("\n[TEST 10] 구조체 값 저장");

    HashMap *map = hash_new(sizeof(int), sizeof(Point),
                           hash_int, cmp_int);

    int key1 = 1;
    int key2 = 2;
    Point point1 = {10, 20};
    Point point2 = {30, 40};

    hash_set(map, &key1, &point1);
    hash_set(map, &key2, &point2);

    assert_size_t("size is 2", hash_size(map), 2);

    Point *p1 = (Point*)hash_get(map, &key1);
    Point *p2 = (Point*)hash_get(map, &key2);

    assert_eq_int("p1.x is 10", p1->x, 10);
    assert_eq_int("p1.y is 20", p1->y, 20);
    assert_eq_int("p2.x is 30", p2->x, 30);
    assert_eq_int("p2.y is 40", p2->y, 40);

    hash_free(map);
}

static void test_hash_dump() {
    test_writeln("\n[TEST 11] hash_dump - 정보 출력");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    for (int i = 0; i < 25; i++) {
        int value = i * 10;
        hash_set(map, &i, &value);
    }

    hash_dump(map);
    test_passed++;
    test_count++;

    hash_free(map);
}

static void test_hash_validate() {
    test_writeln("\n[TEST 12] hash_validate - 무결성 검증");

    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    for (int i = 0; i < 30; i++) {
        int value = i * 10;
        hash_set(map, &i, &value);
    }

    int result = hash_validate(map);
    assert_eq_int("validate returns 0", result, 0);

    hash_free(map);
}

/* ============ Test Runner ============ */

int main(void) {
    test_writeln("╔════════════════════════════════════════╗");
    test_writeln("║  MyOS HashMap Test Suite               ║");
    test_writeln("║  Hash Table (Open Addressing)          ║");
    test_writeln("╚════════════════════════════════════════╝");

    /* Memory Manager 초기화 */
    mm_init(10 * 1024 * 1024);  /* 10MB 힙 */

    /* 테스트 실행 */
    test_hash_new();
    test_hash_set_get();
    test_hash_update();
    test_hash_delete();
    test_hash_contains();
    test_hash_clear();
    test_hash_autogrow();
    test_hash_collisions();
    test_hash_string_keys();
    test_hash_struct_values();
    test_hash_dump();
    test_hash_validate();

    /* Memory Manager 정리 */
    mm_destroy();

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
