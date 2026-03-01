/**
 * MyOS_Lib Linked List Test Suite (test_list.c)
 *
 * Phase B-4 테스트
 */

#include "src/list.h"
#include "src/mm.h"
#include <stdio.h>

/* ============================================================================
 * 테스트 헬퍼
 * ============================================================================ */

static int test_count = 0;
static int test_passed = 0;

#define ASSERT(name, cond) do { \
    test_count++; \
    if (cond) { \
        printf("  ✅ %s PASS\n", name); \
        test_passed++; \
    } else { \
        printf("  ❌ %s FAIL\n", name); \
    } \
} while(0)

#define TEST(name) \
    printf("\n[TEST %d] %s\n", ++test_count, name)

static int cmp_int(const void *a, const void *b) {
    int va = *(int *)a;
    int vb = *(int *)b;
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

static int is_even(const void *elem) {
    return (*(int *)elem) % 2 == 0;
}

typedef struct {
    int id;
    int value;
} TestItem;

/* ============================================================================
 * 테스트 케이스
 * ============================================================================ */

int main(void) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║  LinkedList Test Suite (Phase B-4)     ║\n");
    printf("╚════════════════════════════════════════╝\n");

    mm_init(1024 * 1024);

    test_count = 0;
    test_passed = 0;

    /* Test 1: 기본 생성/해제 */
    {
        LinkedList *list = list_new(sizeof(int));
        ASSERT("new returns non-NULL", list != NULL);
        ASSERT("initial size is 0", list_size(list) == 0);
        ASSERT("is_empty returns 1", list_is_empty(list) == 1);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 2: push_back */
    {
        LinkedList *list = list_new(sizeof(int));
        int v1 = 10, v2 = 20, v3 = 30;
        ASSERT("push_back 1 returns 0", list_push_back(list, &v1) == 0);
        ASSERT("push_back 2 returns 0", list_push_back(list, &v2) == 0);
        ASSERT("push_back 3 returns 0", list_push_back(list, &v3) == 0);
        ASSERT("size is 3", list_size(list) == 3);
        ASSERT("is_empty returns 0", list_is_empty(list) == 0);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 3: push_front */
    {
        LinkedList *list = list_new(sizeof(int));
        int v1 = 100, v2 = 200;
        list_push_front(list, &v1);
        list_push_front(list, &v2);
        int *front = (int *)list_front(list);
        ASSERT("front is 200", front && *front == 200);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 4: list_at (접근) */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 0; i < 5; i++) {
            list_push_back(list, &i);
        }
        int *elem0 = (int *)list_at(list, 0);
        int *elem2 = (int *)list_at(list, 2);
        int *elem4 = (int *)list_at(list, 4);
        ASSERT("at(0) is 0", elem0 && *elem0 == 0);
        ASSERT("at(2) is 2", elem2 && *elem2 == 2);
        ASSERT("at(4) is 4", elem4 && *elem4 == 4);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 5: pop_front */
    {
        LinkedList *list = list_new(sizeof(int));
        int v1 = 111, v2 = 222, v3 = 333;
        list_push_back(list, &v1);
        list_push_back(list, &v2);
        list_push_back(list, &v3);

        int out;
        ASSERT("pop_front returns 0", list_pop_front(list, &out) == 0);
        ASSERT("popped value is 111", out == 111);
        ASSERT("size is 2", list_size(list) == 2);

        int *front = (int *)list_front(list);
        ASSERT("new front is 222", front && *front == 222);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 6: pop_back */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 1; i <= 3; i++) {
            list_push_back(list, &i);
        }

        int out;
        list_pop_back(list, &out);
        ASSERT("popped value is 3", out == 3);
        ASSERT("size is 2", list_size(list) == 2);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 7: insert */
    {
        LinkedList *list = list_new(sizeof(int));
        int v[] = {10, 30};
        list_push_back(list, &v[0]);
        list_push_back(list, &v[1]);

        int v_insert = 20;
        ASSERT("insert at 1 returns 0", list_insert(list, 1, &v_insert) == 0);
        ASSERT("size is 3", list_size(list) == 3);

        int *middle = (int *)list_at(list, 1);
        ASSERT("middle element is 20", middle && *middle == 20);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 8: remove */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 10; i <= 50; i += 10) {
            list_push_back(list, &i);
        }

        int out;
        ASSERT("remove at 2 returns 0", list_remove(list, 2, &out) == 0);
        ASSERT("removed value is 30", out == 30);
        ASSERT("size is 4", list_size(list) == 4);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 9: list_find */
    {
        LinkedList *list = list_new(sizeof(int));
        int v[] = {5, 15, 25, 35, 45};
        for (int i = 0; i < 5; i++) {
            list_push_back(list, &v[i]);
        }

        int target = 25;
        int idx = list_find(list, &target, cmp_int);
        ASSERT("find 25 returns 2", idx == 2);

        int notfound = 99;
        idx = list_find(list, &notfound, cmp_int);
        ASSERT("find 99 returns -1", idx == -1);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 10: list_find_if */
    {
        LinkedList *list = list_new(sizeof(int));
        int v[] = {1, 2, 3, 4, 5};
        for (int i = 0; i < 5; i++) {
            list_push_back(list, &v[i]);
        }

        int idx = list_find_if(list, is_even);
        ASSERT("find first even is 1", idx == 1);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 11: list_foreach */
    {
        LinkedList *list = list_new(sizeof(int));
        int sum = 0;
        for (int i = 1; i <= 10; i++) {
            list_push_back(list, &i);
            sum += i;
        }

        ASSERT("size is 10", list_size(list) == 10);
        int *front = (int *)list_front(list);
        int *back = (int *)list_back(list);
        ASSERT("front is 1", front && *front == 1);
        ASSERT("back is 10", back && *back == 10);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 12: list_reverse */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 1; i <= 4; i++) {
            list_push_back(list, &i);
        }

        list_reverse(list);
        int *front = (int *)list_front(list);
        int *back = (int *)list_back(list);
        ASSERT("reversed front is 4", front && *front == 4);
        ASSERT("reversed back is 1", back && *back == 1);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 13: list_remove_if */
    {
        LinkedList *list = list_new(sizeof(int));
        int v[] = {1, 2, 3, 4, 5, 6};
        for (int i = 0; i < 6; i++) {
            list_push_back(list, &v[i]);
        }

        size_t removed = list_remove_if(list, is_even);
        ASSERT("removed 3 even numbers", removed == 3);
        ASSERT("size is 3", list_size(list) == 3);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 14: list_clear */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 0; i < 10; i++) {
            list_push_back(list, &i);
        }

        ASSERT("size before clear is 10", list_size(list) == 10);
        list_clear(list);
        ASSERT("size after clear is 0", list_size(list) == 0);
        ASSERT("is_empty returns 1", list_is_empty(list) == 1);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 15: Iterator */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 10; i <= 40; i += 10) {
            list_push_back(list, &i);
        }

        ListIterator it = list_iterator(list);
        ASSERT("iterator valid at start", list_iterator_valid(&it));

        int *val = (int *)list_iterator_value(&it);
        ASSERT("first value is 10", val && *val == 10);
        ASSERT("first index is 0", list_iterator_index(&it) == 0);

        list_iterator_next(&it);
        val = (int *)list_iterator_value(&it);
        ASSERT("second value is 20", val && *val == 20);

        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 16: Struct 저장 */
    {
        LinkedList *list = list_new(sizeof(TestItem));
        TestItem item1 = {1, 100};
        TestItem item2 = {2, 200};

        list_push_back(list, &item1);
        list_push_back(list, &item2);

        TestItem *retrieved = (TestItem *)list_at(list, 0);
        ASSERT("struct id is 1", retrieved && retrieved->id == 1);
        ASSERT("struct value is 100", retrieved && retrieved->value == 100);

        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 17: list_merge */
    {
        LinkedList *list1 = list_new(sizeof(int));
        LinkedList *list2 = list_new(sizeof(int));

        for (int i = 1; i <= 3; i++) {
            list_push_back(list1, &i);
        }
        for (int i = 4; i <= 6; i++) {
            list_push_back(list2, &i);
        }

        ASSERT("merge returns 0", list_merge(list1, list2) == 0);
        ASSERT("list1 size is 6", list_size(list1) == 6);
        ASSERT("list2 size is 0", list_size(list2) == 0);

        list_free(list1);
        list_free(list2);
        ASSERT("free succeeds", 1);
    }

    /* Test 18: list_validate */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 0; i < 10; i++) {
            list_push_back(list, &i);
        }

        ASSERT("validate returns 0", list_validate(list) == 0);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 19: Big list performance */
    {
        LinkedList *list = list_new(sizeof(int));
        for (int i = 0; i < 1000; i++) {
            int val = i;
            list_push_back(list, &val);
        }

        ASSERT("size is 1000", list_size(list) == 1000);

        int *elem500 = (int *)list_at(list, 500);
        ASSERT("elem 500 exists", elem500 != NULL);

        list_clear(list);
        ASSERT("cleared size is 0", list_size(list) == 0);
        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* Test 20: Edge cases */
    {
        LinkedList *list = list_new(sizeof(int));

        /* Pop from empty */
        int out;
        ASSERT("pop_front empty returns -1", list_pop_front(list, &out) == -1);
        ASSERT("pop_back empty returns -1", list_pop_back(list, &out) == -1);

        /* Remove from empty */
        ASSERT("remove empty returns -1", list_remove(list, 0, &out) == -1);

        /* At out of range */
        int v = 42;
        list_push_back(list, &v);
        ASSERT("at(5) returns NULL", list_at(list, 5) == NULL);

        list_free(list);
        ASSERT("free succeeds", 1);
    }

    /* ========== Summary ========== */
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║  Test Summary                          ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("Total Tests: %d\n", test_count);
    printf("✅ Passed: %d\n", test_passed);
    printf("❌ Failed: %d\n", test_count - test_passed);

    if (test_passed == test_count) {
        printf("\n🎉 All tests passed!\n");
    }

    mm_destroy();

    return test_passed == test_count ? 0 : 1;
}
