/**
 * MyOS_Lib Linked List Implementation (list.c)
 *
 * Zero-Dependency 양방향 연결 리스트
 * - Memory Manager 기반
 * - Generic void* 타입
 * - Iterator 지원
 */

#include "list.h"
#include "mm.h"
#include <stdint.h>

/* ============================================================================
 * 내부 헬퍼 함수
 * ============================================================================ */

/**
 * 메모리 복사
 */
static void* my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

/**
 * 메모리 비교
 */
static int my_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] < p2[i]) return -1;
        if (p1[i] > p2[i]) return 1;
    }
    return 0;
}

/**
 * 문자열 길이
 */
static size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s && s[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * syscall을 통한 디버그 출력
 */
static void debug_write(const char *msg) {
    size_t len = my_strlen(msg);
    if (len > 0) {
        syscall(1, 2, (long)msg, (long)len, 0, 0, 0);  /* write(2, msg, len) */
    }
}

/**
 * 노드 생성
 */
static ListNode* list_node_new(const void *elem, size_t elem_size) {
    ListNode *node = (ListNode *)mm_alloc(sizeof(ListNode));
    if (!node) return NULL;

    if (elem_size > 0 && elem) {
        node->data = mm_alloc(elem_size);
        if (!node->data) {
            mm_free(node);
            return NULL;
        }
        my_memcpy(node->data, elem, elem_size);
    } else {
        node->data = (void *)elem;
    }

    node->next = NULL;
    node->prev = NULL;

    return node;
}

/**
 * 노드 해제
 */
static void list_node_free(ListNode *node, size_t elem_size) {
    if (node) {
        if (elem_size > 0 && node->data) {
            mm_free(node->data);
        }
        mm_free(node);
    }
}

/**
 * 인덱스로 노드 찾기
 */
static ListNode* list_find_node(LinkedList *list, size_t index) {
    if (!list || index >= list->count) return NULL;

    ListNode *node;
    if (index < list->count / 2) {
        /* 앞에서부터 탐색 */
        node = list->head;
        for (size_t i = 0; i < index; i++) {
            if (!node) return NULL;
            node = node->next;
        }
    } else {
        /* 뒤에서부터 탐색 */
        node = list->tail;
        for (size_t i = list->count - 1; i > index; i--) {
            if (!node) return NULL;
            node = node->prev;
        }
    }

    return node;
}

/* ============================================================================
 * 생성/소멸
 * ============================================================================ */

LinkedList* list_new(size_t elem_size) {
    LinkedList *list = (LinkedList *)mm_alloc(sizeof(LinkedList));
    if (!list) return NULL;

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    list->elem_size = elem_size;

    return list;
}

void list_free(LinkedList *list) {
    if (!list) return;

    ListNode *node = list->head;
    while (node) {
        ListNode *next = node->next;
        list_node_free(node, list->elem_size);
        node = next;
    }

    mm_free(list);
}

/* ============================================================================
 * 데이터 추가/제거
 * ============================================================================ */

int list_push_front(LinkedList *list, const void *elem) {
    if (!list) return -1;

    ListNode *node = list_node_new(elem, list->elem_size);
    if (!node) return -1;

    if (list->head) {
        node->next = list->head;
        list->head->prev = node;
    } else {
        list->tail = node;
    }

    list->head = node;
    list->count++;

    return 0;
}

int list_push_back(LinkedList *list, const void *elem) {
    if (!list) return -1;

    ListNode *node = list_node_new(elem, list->elem_size);
    if (!node) return -1;

    if (list->tail) {
        node->prev = list->tail;
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
    list->count++;

    return 0;
}

int list_pop_front(LinkedList *list, void *out) {
    if (!list || !list->head) return -1;

    ListNode *node = list->head;
    if (out && list->elem_size > 0) {
        my_memcpy(out, node->data, list->elem_size);
    }

    list->head = node->next;
    if (list->head) {
        list->head->prev = NULL;
    } else {
        list->tail = NULL;
    }

    list_node_free(node, list->elem_size);
    list->count--;

    return 0;
}

int list_pop_back(LinkedList *list, void *out) {
    if (!list || !list->tail) return -1;

    ListNode *node = list->tail;
    if (out && list->elem_size > 0) {
        my_memcpy(out, node->data, list->elem_size);
    }

    list->tail = node->prev;
    if (list->tail) {
        list->tail->next = NULL;
    } else {
        list->head = NULL;
    }

    list_node_free(node, list->elem_size);
    list->count--;

    return 0;
}

int list_insert(LinkedList *list, size_t index, const void *elem) {
    if (!list || index > list->count) return -1;

    if (index == 0) {
        return list_push_front(list, elem);
    }

    if (index == list->count) {
        return list_push_back(list, elem);
    }

    ListNode *new_node = list_node_new(elem, list->elem_size);
    if (!new_node) return -1;

    ListNode *after = list_find_node(list, index);
    if (!after || !after->prev) {
        list_node_free(new_node, list->elem_size);
        return -1;
    }

    ListNode *before = after->prev;
    new_node->next = after;
    new_node->prev = before;
    before->next = new_node;
    after->prev = new_node;

    list->count++;

    return 0;
}

int list_remove(LinkedList *list, size_t index, void *out) {
    if (!list || index >= list->count) return -1;

    ListNode *node = list_find_node(list, index);
    if (!node) return -1;

    if (out && list->elem_size > 0) {
        my_memcpy(out, node->data, list->elem_size);
    }

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        list->tail = node->prev;
    }

    list_node_free(node, list->elem_size);
    list->count--;

    return 0;
}

void list_clear(LinkedList *list) {
    if (!list) return;

    ListNode *node = list->head;
    while (node) {
        ListNode *next = node->next;
        list_node_free(node, list->elem_size);
        node = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

/* ============================================================================
 * 쿼리/접근
 * ============================================================================ */

size_t list_size(const LinkedList *list) {
    return list ? list->count : 0;
}

int list_is_empty(const LinkedList *list) {
    return !list || list->count == 0;
}

void* list_at(LinkedList *list, size_t index) {
    ListNode *node = list_find_node(list, index);
    return node ? node->data : NULL;
}

int list_set(LinkedList *list, size_t index, const void *elem) {
    if (!list || index >= list->count) return -1;

    ListNode *node = list_find_node(list, index);
    if (!node || !elem) return -1;

    if (list->elem_size > 0) {
        my_memcpy(node->data, elem, list->elem_size);
    }

    return 0;
}

void* list_front(LinkedList *list) {
    return list && list->head ? list->head->data : NULL;
}

void* list_back(LinkedList *list) {
    return list && list->tail ? list->tail->data : NULL;
}

/* ============================================================================
 * 검색/비교
 * ============================================================================ */

int list_find(LinkedList *list, const void *elem,
              int (*cmp)(const void*, const void*)) {
    if (!list || !elem || !cmp) return -1;

    ListNode *node = list->head;
    size_t index = 0;

    while (node) {
        if (cmp(node->data, elem) == 0) {
            return (int)index;
        }
        node = node->next;
        index++;
    }

    return -1;
}

int list_find_if(LinkedList *list,
                 int (*predicate)(const void*)) {
    if (!list || !predicate) return -1;

    ListNode *node = list->head;
    size_t index = 0;

    while (node) {
        if (predicate(node->data)) {
            return (int)index;
        }
        node = node->next;
        index++;
    }

    return -1;
}

/* ============================================================================
 * 순회
 * ============================================================================ */

void list_foreach(LinkedList *list,
                  void (*callback)(void*, size_t)) {
    if (!list || !callback) return;

    ListNode *node = list->head;
    size_t index = 0;

    while (node) {
        callback(node->data, index);
        node = node->next;
        index++;
    }
}

void list_foreach_reverse(LinkedList *list,
                          void (*callback)(void*, size_t)) {
    if (!list || !callback) return;

    ListNode *node = list->tail;
    int index = (int)list->count - 1;

    while (node && index >= 0) {
        callback(node->data, (size_t)index);
        node = node->prev;
        index--;
    }
}

/* ============================================================================
 * Iterator
 * ============================================================================ */

ListIterator list_iterator(LinkedList *list) {
    ListIterator it;
    it.list = list;
    it.current = list ? list->head : NULL;
    it.index = 0;
    return it;
}

int list_iterator_valid(const ListIterator *it) {
    return it && it->current != NULL;
}

int list_iterator_next(ListIterator *it) {
    if (!it || !it->current) return 0;

    it->current = it->current->next;
    it->index++;

    return it->current != NULL;
}

void* list_iterator_value(const ListIterator *it) {
    return it && it->current ? it->current->data : NULL;
}

size_t list_iterator_index(const ListIterator *it) {
    return it ? it->index : 0;
}

/* ============================================================================
 * 고급 기능
 * ============================================================================ */

void list_reverse(LinkedList *list) {
    if (!list || list->count < 2) return;

    ListNode *node = list->head;
    ListNode *temp;

    while (node) {
        temp = node->next;
        node->next = node->prev;
        node->prev = temp;
        node = temp;
    }

    temp = list->head;
    list->head = list->tail;
    list->tail = temp;
}

size_t list_erase_first_n(LinkedList *list, size_t count) {
    if (!list || count == 0) return 0;

    size_t removed = 0;
    while (list->head && removed < count) {
        list_pop_front(list, NULL);
        removed++;
    }

    return removed;
}

size_t list_remove_if(LinkedList *list,
                      int (*predicate)(const void*)) {
    if (!list || !predicate) return 0;

    size_t removed = 0;
    ListNode *node = list->head;

    while (node) {
        ListNode *next = node->next;
        if (predicate(node->data)) {
            if (node->prev) {
                node->prev->next = node->next;
            } else {
                list->head = node->next;
            }

            if (node->next) {
                node->next->prev = node->prev;
            } else {
                list->tail = node->prev;
            }

            list_node_free(node, list->elem_size);
            list->count--;
            removed++;
        }

        node = next;
    }

    return removed;
}

size_t list_unique(LinkedList *list,
                   int (*cmp)(const void*, const void*)) {
    if (!list || !cmp || list->count < 2) return 0;

    size_t removed = 0;
    ListNode *node = list->head;

    while (node && node->next) {
        if (cmp(node->data, node->next->data) == 0) {
            ListNode *dup = node->next;
            node->next = dup->next;
            if (dup->next) {
                dup->next->prev = node;
            } else {
                list->tail = node;
            }

            list_node_free(dup, list->elem_size);
            list->count--;
            removed++;
        } else {
            node = node->next;
        }
    }

    return removed;
}

int list_merge(LinkedList *list1, LinkedList *list2) {
    if (!list1 || !list2 || list1->elem_size != list2->elem_size) {
        return -1;
    }

    if (!list2->head) return 0;

    if (!list1->tail) {
        list1->head = list2->head;
        list1->tail = list2->tail;
    } else {
        list1->tail->next = list2->head;
        list2->head->prev = list1->tail;
        list1->tail = list2->tail;
    }

    list1->count += list2->count;

    list2->head = NULL;
    list2->tail = NULL;
    list2->count = 0;

    return 0;
}

/* ============================================================================
 * 디버그
 * ============================================================================ */

void list_dump(const LinkedList *list, const char *label) {
    if (!list) return;

    char buf[256];
    int len = 0;
    const char *p = "List[%s]: count=%lu, elem_size=%lu\n";
    int arg_idx = 0;

    while (*p && len < 255) {
        if (p[0] == '%' && p[1] == 's') {
            const char *arg = (arg_idx == 0) ? label : "";
            arg_idx++;
            while (*arg && len < 255) {
                buf[len++] = *arg++;
            }
            p += 2;
        } else if (p[0] == '%' && p[1] == 'l' && p[2] == 'u') {
            size_t val = (arg_idx == 1) ? list->count : list->elem_size;
            arg_idx++;
            char nbuf[32];
            size_t nlen = 0;
            size_t tmp = val;
            if (tmp == 0) nbuf[nlen++] = '0';
            else {
                while (tmp > 0) {
                    nbuf[nlen++] = '0' + (tmp % 10);
                    tmp /= 10;
                }
                for (size_t i = 0; i < nlen / 2; i++) {
                    char t = nbuf[i];
                    nbuf[i] = nbuf[nlen - 1 - i];
                    nbuf[nlen - 1 - i] = t;
                }
            }
            for (size_t i = 0; i < nlen && len < 255; i++) {
                buf[len++] = nbuf[i];
            }
            p += 3;
        } else {
            buf[len++] = *p++;
        }
    }
    buf[len] = '\0';

    debug_write(buf);
}

int list_validate(const LinkedList *list) {
    if (!list) return -1;

    if (list->count == 0) {
        if (list->head != NULL || list->tail != NULL) return -1;
        return 0;
    }

    if (!list->head || !list->tail) return -1;

    /* forward 검증 */
    size_t forward_count = 0;
    ListNode *node = list->head;
    ListNode *prev = NULL;

    while (node) {
        if (node->prev != prev) return -1;
        prev = node;
        node = node->next;
        forward_count++;
    }

    if (prev != list->tail) return -1;

    /* backward 검증 */
    size_t backward_count = 0;
    node = list->tail;
    prev = NULL;

    while (node) {
        if (node->next != prev) return -1;
        prev = node;
        node = node->prev;
        backward_count++;
    }

    if (prev != list->head) return -1;

    /* count 일치 확인 */
    if (forward_count != list->count || backward_count != list->count) {
        return -1;
    }

    return 0;
}
