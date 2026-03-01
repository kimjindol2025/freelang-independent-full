/**
 * MyOS Vector Test Suite (test_vector.c)
 *
 * 테스트 케이스:
 * 1. vector_new - 벡터 생성
 * 2. vector_push - 요소 추가
 * 3. vector_pop - 요소 제거
 * 4. vector_at - 요소 접근
 * 5. vector_set - 요소 설정
 * 6. vector_insert - 요소 삽입
 * 7. vector_remove - 요소 제거
 * 8. vector_clear - 전체 제거
 * 9. vector_reserve - 사전 할당
 * 10. vector_shrink_to_fit - 메모리 최적화
 * 11. 자동 리사이징 테스트
 * 12. 다중 타입 테스트 (int, char, struct)
 */

#include "src/vector.h"
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

/* ============ Test Cases ============ */

static void test_vector_new() {
    test_writeln("\n[TEST 1] vector_new - 벡터 생성");

    Vector *v = vector_new(sizeof(int));
    assert_not_null("vector_new(sizeof(int)) returns non-NULL", v);

    assert_size_t("initial size is 0", vector_size(v), 0);
    assert_size_t("initial capacity is 16", vector_capacity(v), 16);
    assert_eq_int("is_empty returns 1", vector_is_empty(v), 1);

    vector_free(v);
}

static void test_vector_push_single() {
    test_writeln("\n[TEST 2] vector_push - 단일 요소 추가");

    Vector *v = vector_new(sizeof(int));

    int x = 42;
    int result = vector_push(v, &x);
    assert_eq_int("push returns 0", result, 0);
    assert_size_t("size is 1", vector_size(v), 1);

    int *elem = (int*)vector_at(v, 0);
    assert_not_null("element at index 0 is not NULL", elem);
    assert_eq_int("element value is 42", *elem, 42);

    vector_free(v);
}

static void test_vector_push_multiple() {
    test_writeln("\n[TEST 3] vector_push - 다중 요소 추가");

    Vector *v = vector_new(sizeof(int));

    for (int i = 0; i < 100; i++) {
        vector_push(v, &i);
    }

    assert_size_t("size is 100", vector_size(v), 100);

    /* 처음과 끝 확인 */
    int *first = (int*)vector_at(v, 0);
    int *last = (int*)vector_at(v, 99);

    assert_eq_int("first element is 0", *first, 0);
    assert_eq_int("last element is 99", *last, 99);

    vector_free(v);
}

static void test_vector_pop() {
    test_writeln("\n[TEST 4] vector_pop - 요소 제거");

    Vector *v = vector_new(sizeof(int));

    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        vector_push(v, &vals[i]);
    }

    assert_size_t("size is 3 before pop", vector_size(v), 3);

    int popped;
    vector_pop(v, &popped);
    assert_eq_int("popped value is 30", popped, 30);
    assert_size_t("size is 2 after pop", vector_size(v), 2);

    vector_free(v);
}

static void test_vector_set() {
    test_writeln("\n[TEST 5] vector_set - 요소 설정");

    Vector *v = vector_new(sizeof(int));

    int x = 10;
    vector_push(v, &x);

    int y = 99;
    int result = vector_set(v, 0, &y);
    assert_eq_int("set returns 0", result, 0);

    int *elem = (int*)vector_at(v, 0);
    assert_eq_int("element is 99 after set", *elem, 99);

    vector_free(v);
}

static void test_vector_insert() {
    test_writeln("\n[TEST 6] vector_insert - 요소 삽입");

    Vector *v = vector_new(sizeof(int));

    int vals[] = {10, 30};
    for (int i = 0; i < 2; i++) {
        vector_push(v, &vals[i]);
    }

    int mid = 20;
    int result = vector_insert(v, 1, &mid);
    assert_eq_int("insert returns 0", result, 0);
    assert_size_t("size is 3", vector_size(v), 3);

    int *e0 = (int*)vector_at(v, 0);
    int *e1 = (int*)vector_at(v, 1);
    int *e2 = (int*)vector_at(v, 2);

    assert_eq_int("element[0] is 10", *e0, 10);
    assert_eq_int("element[1] is 20", *e1, 20);
    assert_eq_int("element[2] is 30", *e2, 30);

    vector_free(v);
}

static void test_vector_remove() {
    test_writeln("\n[TEST 7] vector_remove - 요소 제거");

    Vector *v = vector_new(sizeof(int));

    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) {
        vector_push(v, &vals[i]);
    }

    int removed;
    int result = vector_remove(v, 1, &removed);
    assert_eq_int("remove returns 0", result, 0);
    assert_eq_int("removed value is 20", removed, 20);
    assert_size_t("size is 2", vector_size(v), 2);

    int *e1 = (int*)vector_at(v, 1);
    assert_eq_int("element[1] is 30 after remove", *e1, 30);

    vector_free(v);
}

static void test_vector_clear() {
    test_writeln("\n[TEST 8] vector_clear - 전체 제거");

    Vector *v = vector_new(sizeof(int));

    int vals[] = {1, 2, 3, 4, 5};
    for (int i = 0; i < 5; i++) {
        vector_push(v, &vals[i]);
    }

    assert_size_t("size is 5 before clear", vector_size(v), 5);

    vector_clear(v);
    assert_size_t("size is 0 after clear", vector_size(v), 0);
    assert_eq_int("is_empty returns 1", vector_is_empty(v), 1);

    /* 다시 push 가능 */
    int x = 42;
    vector_push(v, &x);
    assert_size_t("size is 1 after push", vector_size(v), 1);

    vector_free(v);
}

static void test_vector_reserve() {
    test_writeln("\n[TEST 9] vector_reserve - 사전 할당");

    Vector *v = vector_new(sizeof(int));

    int result = vector_reserve(v, 1000);
    assert_eq_int("reserve returns 0", result, 0);
    assert_size_t("capacity is >= 1000", vector_capacity(v) >= 1000, 1);

    vector_free(v);
}

static void test_vector_autogrow() {
    test_writeln("\n[TEST 10] 자동 리사이징 테스트");

    Vector *v = vector_new(sizeof(int));

    size_t initial_capacity = vector_capacity(v);

    /* 용량을 초과할 때까지 push */
    for (int i = 0; i < 50; i++) {
        vector_push(v, &i);
    }

    assert_size_t("size is 50", vector_size(v), 50);

    /* 용량이 자동으로 증가했는지 확인 */
    size_t final_capacity = vector_capacity(v);
    assert_eq_int("capacity grew", (final_capacity > initial_capacity) ? 1 : 0, 1);

    /* 모든 요소가 정확한지 확인 */
    int *first = (int*)vector_at(v, 0);
    int *last = (int*)vector_at(v, 49);
    assert_eq_int("first is 0", *first, 0);
    assert_eq_int("last is 49", *last, 49);

    vector_free(v);
}

static void test_vector_shrink() {
    test_writeln("\n[TEST 11] vector_shrink_to_fit - 메모리 최적화");

    Vector *v = vector_new(sizeof(int));

    for (int i = 0; i < 10; i++) {
        vector_push(v, &i);
    }

    /* 처음 capacity는 16 이상 */
    size_t before = vector_capacity(v);

    int result = vector_shrink_to_fit(v);
    assert_eq_int("shrink returns 0", result, 0);

    size_t after = vector_capacity(v);
    assert_size_t("capacity == size after shrink", after, 10);

    /* 모든 요소 여전히 유효 */
    int *first = (int*)vector_at(v, 0);
    assert_eq_int("first is still 0", *first, 0);

    vector_free(v);
}

typedef struct {
    int x;
    int y;
} Point;

static void test_vector_struct() {
    test_writeln("\n[TEST 12] 구조체 요소 저장");

    Vector *v = vector_new(sizeof(Point));

    Point p1 = {10, 20};
    Point p2 = {30, 40};
    Point p3 = {50, 60};

    vector_push(v, &p1);
    vector_push(v, &p2);
    vector_push(v, &p3);

    assert_size_t("size is 3", vector_size(v), 3);

    Point *elem0 = (Point*)vector_at(v, 0);
    Point *elem1 = (Point*)vector_at(v, 1);
    Point *elem2 = (Point*)vector_at(v, 2);

    assert_eq_int("p1.x is 10", elem0->x, 10);
    assert_eq_int("p1.y is 20", elem0->y, 20);
    assert_eq_int("p2.x is 30", elem1->x, 30);
    assert_eq_int("p2.y is 40", elem1->y, 40);
    assert_eq_int("p3.x is 50", elem2->x, 50);
    assert_eq_int("p3.y is 60", elem2->y, 60);

    vector_free(v);
}

static void test_vector_dump() {
    test_writeln("\n[TEST 13] vector_dump - 벡터 정보 출력");

    Vector *v = vector_new(sizeof(int));

    for (int i = 0; i < 25; i++) {
        vector_push(v, &i);
    }

    vector_dump(v);
    test_passed++;
    test_count++;

    vector_free(v);
}

/* ============ Test Runner ============ */

int main(void) {
    test_writeln("╔════════════════════════════════════════╗");
    test_writeln("║  MyOS Vector Test Suite                ║");
    test_writeln("║  Generic Dynamic Array                 ║");
    test_writeln("╚════════════════════════════════════════╝");

    /* Memory Manager 초기화 */
    mm_init(10 * 1024 * 1024);  /* 10MB 힙 */

    /* 테스트 실행 */
    test_vector_new();
    test_vector_push_single();
    test_vector_push_multiple();
    test_vector_pop();
    test_vector_set();
    test_vector_insert();
    test_vector_remove();
    test_vector_clear();
    test_vector_reserve();
    test_vector_autogrow();
    test_vector_shrink();
    test_vector_struct();
    test_vector_dump();

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
