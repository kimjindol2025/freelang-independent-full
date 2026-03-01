#include <stdio.h>
#include "src/mm.h"
#include "src/object_pool.h"

static int passed = 0, failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); passed++; } \
} while(0)

typedef struct {
    int value;
    char name[32];
} TestObject;

void test_pool_allocate(void) {
    TEST("D-3.1: Object allocation");

    ObjectPool *pool = pool_new(10);
    ASSERT(pool != NULL, "Pool created");
    ASSERT(pool_count(pool) == 0, "Initial count is 0");

    /* 첫 번째 객체 할당 */
    TestObject obj1 = {.value = 42};
    size_t id1 = 0;
    int ret = pool_allocate(pool, &obj1, &id1);
    ASSERT(ret == 0, "Object allocated");
    ASSERT(id1 == 0, "ID is 0");
    ASSERT(pool_count(pool) == 1, "Count is 1");

    /* 두 번째 객체 할당 */
    TestObject obj2 = {.value = 99};
    size_t id2 = 0;
    ret = pool_allocate(pool, &obj2, &id2);
    ASSERT(ret == 0, "Second object allocated");
    ASSERT(id2 == 1, "ID is 1");
    ASSERT(pool_count(pool) == 2, "Count is 2");

    pool_free(pool);
}

void test_pool_get(void) {
    TEST("D-3.2: Object retrieval");

    ObjectPool *pool = pool_new(10);

    TestObject obj1 = {.value = 100};
    TestObject obj2 = {.value = 200};

    size_t id1 = 0, id2 = 0;
    pool_allocate(pool, &obj1, &id1);
    pool_allocate(pool, &obj2, &id2);

    /* 객체 조회 */
    TestObject *retrieved1 = (TestObject *)pool_get(pool, id1);
    ASSERT(retrieved1 != NULL, "Object retrieved");
    ASSERT(retrieved1->value == 100, "Value matches");

    TestObject *retrieved2 = (TestObject *)pool_get(pool, id2);
    ASSERT(retrieved2 != NULL, "Second object retrieved");
    ASSERT(retrieved2->value == 200, "Value matches");

    /* 유효하지 않은 ID */
    void *invalid = pool_get(pool, 999);
    ASSERT(invalid == NULL, "Invalid ID returns NULL");

    pool_free(pool);
}

void test_pool_release(void) {
    TEST("D-3.3: Object release");

    ObjectPool *pool = pool_new(10);

    TestObject obj1 = {.value = 42};
    size_t id1 = 0;
    pool_allocate(pool, &obj1, &id1);

    ASSERT(pool_count(pool) == 1, "Count is 1 after allocate");
    ASSERT(pool_is_allocated(pool, id1) == 1, "Object is allocated");

    /* 객체 해제 */
    int ret = pool_release(pool, id1);
    ASSERT(ret == 0, "Release successful");
    ASSERT(pool_count(pool) == 0, "Count is 0 after release");
    ASSERT(pool_is_allocated(pool, id1) == 0, "Object is not allocated");

    /* 이미 해제된 객체 다시 해제 */
    ret = pool_release(pool, id1);
    ASSERT(ret == -1, "Cannot release unallocated object");

    pool_free(pool);
}

void test_pool_reuse_id(void) {
    TEST("D-3.4: ID reuse");

    ObjectPool *pool = pool_new(10);

    TestObject obj1 = {.value = 100};
    TestObject obj2 = {.value = 200};
    TestObject obj3 = {.value = 300};

    /* ID 0, 1 할당 */
    size_t id1 = 0, id2 = 0, id3 = 0;
    pool_allocate(pool, &obj1, &id1);
    pool_allocate(pool, &obj2, &id2);

    ASSERT(id1 == 0 && id2 == 1, "IDs are sequential");

    /* ID 0 해제 */
    pool_release(pool, id1);
    ASSERT(pool_count(pool) == 1, "Count is 1");

    /* ID 0 재할당 (재사용) */
    pool_allocate(pool, &obj3, &id3);
    ASSERT(id3 == 0, "ID 0 reused");
    ASSERT(pool_count(pool) == 2, "Count is 2");

    /* 재할당된 객체 확인 */
    TestObject *retrieved = (TestObject *)pool_get(pool, id3);
    ASSERT(retrieved->value == 300, "New object in reused ID");

    pool_free(pool);
}

void test_pool_capacity(void) {
    TEST("D-3.5: Pool capacity management");

    ObjectPool *pool = pool_new(3);
    ASSERT(pool != NULL, "Pool created with capacity 3");

    TestObject obj1 = {.value = 1};
    TestObject obj2 = {.value = 2};
    TestObject obj3 = {.value = 3};
    TestObject obj4 = {.value = 4};

    size_t id1 = 0, id2 = 0, id3 = 0, id4 = 0;

    /* 3개까지 할당 가능 */
    int ret1 = pool_allocate(pool, &obj1, &id1);
    int ret2 = pool_allocate(pool, &obj2, &id2);
    int ret3 = pool_allocate(pool, &obj3, &id3);
    ASSERT(ret1 == 0 && ret2 == 0 && ret3 == 0, "3 objects allocated");
    ASSERT(pool_count(pool) == 3, "Count is 3");

    /* 4개째는 실패 */
    int ret4 = pool_allocate(pool, &obj4, &id4);
    ASSERT(ret4 == -1, "4th object allocation fails");
    ASSERT(pool_count(pool) == 3, "Count remains 3");

    /* 1개 해제 후 다시 할당 */
    pool_release(pool, id1);
    ASSERT(pool_count(pool) == 2, "Count is 2 after release");

    ret4 = pool_allocate(pool, &obj4, &id4);
    ASSERT(ret4 == 0, "4th object allocated after release");
    ASSERT(pool_count(pool) == 3, "Count is 3 again");

    pool_free(pool);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase D-3: Object Pool - 5 Tests\n");
    printf("═════════════════════════════════════════════════════\n");

    mm_init(8 * 1024 * 1024);

    test_pool_allocate();
    test_pool_get();
    test_pool_release();
    test_pool_reuse_id();
    test_pool_capacity();

    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");

    return (failed > 0) ? 1 : 0;
}
