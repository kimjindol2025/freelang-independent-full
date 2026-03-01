/**
 * MyOS Vector (vector.c)
 *
 * 동적 배열 구현 (Generic container)
 * - Memory Manager (mm.c) 기반
 * - 자동 리사이징 (2배 확장)
 * - 제너릭 요소 저장 (void* 기반)
 */

#include "vector.h"
#include "mm.h"
#include <sys/syscall.h>
#include <string.h>

/* ============ Internal Utilities ============ */

/**
 * memmove - 메모리 이동 (시스템 libc 사용)
 *
 * 겹칠 수 있는 메모리 영역 복사
 * 주의: libc 버전 사용 (mm.c는 libc 함수 안 씀)
 */
static void* my_memmove(void *dest, const void *src, size_t n) {
    /* 겹치는 경우: 뒤에서 앞으로 복사 */
    if (dest > src) {
        for (size_t i = n; i > 0; i--) {
            ((char *)dest)[i - 1] = ((char *)src)[i - 1];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            ((char *)dest)[i] = ((char *)src)[i];
        }
    }
    return dest;
}

/**
 * my_memcpy - 메모리 복사
 */
static void* my_memcpy(void *dest, const void *src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char *)dest)[i] = ((char *)src)[i];
    }
    return dest;
}

/**
 * syscall_write - 디버그 출력
 */
static void debug_write(const char *msg) {
    size_t len = 0;
    while (msg[len]) len++;
    syscall(SYS_write, 1, msg, len);
}

/**
 * itoa_simple - 정수를 문자열로 변환
 */
static int itoa_simple(int64_t num, char *buf, int base) {
    if (num == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return 1;
    }

    int is_negative = (num < 0);
    if (is_negative) num = -num;

    int len = 0;
    int64_t temp = num;
    while (temp > 0) {
        len++;
        temp /= base;
    }

    int total_len = len + (is_negative ? 1 : 0);
    int idx = total_len - 1;

    while (num > 0) {
        int digit = num % base;
        buf[idx--] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
        num /= base;
    }

    if (is_negative) buf[0] = '-';
    buf[total_len] = '\0';

    return total_len;
}

/* ============ Vector Internal ============ */

/**
 * vector_grow - 벡터 용량 확대
 *
 * 새로운 용량: max(current * 2, 16)
 */
static int vector_grow(Vector *v, size_t min_capacity) {
    if (v->capacity >= min_capacity) {
        return 0;  /* 이미 충분함 */
    }

    /* 새로운 용량 계산 (2배 또는 최소 16) */
    size_t new_capacity = v->capacity * 2;
    if (new_capacity < 16) new_capacity = 16;
    if (new_capacity < min_capacity) new_capacity = min_capacity;

    /* 새로운 메모리 할당 */
    void *new_data = mm_alloc(new_capacity * v->elem_size);
    if (new_data == NULL) {
        return -1;
    }

    /* 기존 데이터 복사 */
    if (v->data != NULL && v->count > 0) {
        my_memcpy(new_data, v->data, v->count * v->elem_size);
        mm_free(v->data);
    }

    v->data = new_data;
    v->capacity = new_capacity;
    return 0;
}

/* ============ Vector API Implementation ============ */

/**
 * vector_new - 새 벡터 생성
 */
Vector* vector_new(size_t elem_size) {
    if (elem_size == 0) {
        return NULL;
    }

    /* 벡터 구조체 할당 */
    Vector *v = (Vector *)mm_alloc(sizeof(Vector));
    if (v == NULL) {
        return NULL;
    }

    /* 초기 데이터 버퍼 할당 (16개 요소) */
    v->data = mm_alloc(16 * elem_size);
    if (v->data == NULL) {
        mm_free(v);
        return NULL;
    }

    v->count = 0;
    v->capacity = 16;
    v->elem_size = elem_size;

    return v;
}

/**
 * vector_free - 벡터 정리
 */
void vector_free(Vector *v) {
    if (v == NULL) {
        return;
    }

    if (v->data != NULL) {
        mm_free(v->data);
    }

    mm_free(v);
}

/**
 * vector_push - 요소 추가
 */
int vector_push(Vector *v, const void *elem) {
    if (v == NULL || elem == NULL) {
        return -1;
    }

    /* 용량 확인 */
    if (v->count >= v->capacity) {
        if (vector_grow(v, v->count + 1) < 0) {
            return -1;
        }
    }

    /* 요소 추가 */
    char *dst = (char *)v->data + (v->count * v->elem_size);
    my_memcpy(dst, elem, v->elem_size);
    v->count++;

    return 0;
}

/**
 * vector_pop - 마지막 요소 제거
 */
void vector_pop(Vector *v, void *out) {
    if (v == NULL || v->count == 0) {
        return;
    }

    v->count--;

    /* 제거된 요소 복사 */
    if (out != NULL) {
        char *src = (char *)v->data + (v->count * v->elem_size);
        my_memcpy(out, src, v->elem_size);
    }
}

/**
 * vector_at - 인덱스로 요소 접근
 */
void* vector_at(Vector *v, size_t index) {
    if (v == NULL || index >= v->count) {
        return NULL;
    }

    return (char *)v->data + (index * v->elem_size);
}

/**
 * vector_set - 인덱스의 요소 설정
 */
int vector_set(Vector *v, size_t index, const void *elem) {
    if (v == NULL || elem == NULL || index >= v->count) {
        return -1;
    }

    char *dst = (char *)v->data + (index * v->elem_size);
    my_memcpy(dst, elem, v->elem_size);

    return 0;
}

/**
 * vector_insert - 특정 인덱스에 요소 삽입
 */
int vector_insert(Vector *v, size_t index, const void *elem) {
    if (v == NULL || elem == NULL || index > v->count) {
        return -1;
    }

    /* 용량 확인 */
    if (v->count >= v->capacity) {
        if (vector_grow(v, v->count + 1) < 0) {
            return -1;
        }
    }

    /* 뒤의 요소들을 뒤로 이동 */
    if (index < v->count) {
        char *src = (char *)v->data + (index * v->elem_size);
        char *dst = (char *)v->data + ((index + 1) * v->elem_size);
        my_memmove(dst, src, (v->count - index) * v->elem_size);
    }

    /* 요소 삽입 */
    char *dst = (char *)v->data + (index * v->elem_size);
    my_memcpy(dst, elem, v->elem_size);
    v->count++;

    return 0;
}

/**
 * vector_remove - 특정 인덱스 요소 제거
 */
int vector_remove(Vector *v, size_t index, void *out) {
    if (v == NULL || index >= v->count) {
        return -1;
    }

    /* 제거될 요소 복사 */
    if (out != NULL) {
        char *src = (char *)v->data + (index * v->elem_size);
        my_memcpy(out, src, v->elem_size);
    }

    /* 뒤의 요소들을 앞으로 이동 */
    if (index < v->count - 1) {
        char *src = (char *)v->data + ((index + 1) * v->elem_size);
        char *dst = (char *)v->data + (index * v->elem_size);
        my_memmove(dst, src, (v->count - index - 1) * v->elem_size);
    }

    v->count--;
    return 0;
}

/**
 * vector_clear - 모든 요소 제거
 */
void vector_clear(Vector *v) {
    if (v != NULL) {
        v->count = 0;
    }
}

/* ============ Vector Queries ============ */

/**
 * vector_size - 현재 요소 개수
 */
size_t vector_size(const Vector *v) {
    return (v != NULL) ? v->count : 0;
}

/**
 * vector_capacity - 할당된 용량
 */
size_t vector_capacity(const Vector *v) {
    return (v != NULL) ? v->capacity : 0;
}

/**
 * vector_is_empty - 벡터가 비어있는지 확인
 */
int vector_is_empty(const Vector *v) {
    return (v == NULL) || (v->count == 0);
}

/**
 * vector_get_elem_size - 요소 크기
 */
size_t vector_get_elem_size(const Vector *v) {
    return (v != NULL) ? v->elem_size : 0;
}

/* ============ Vector Utilities ============ */

/**
 * vector_reserve - 사전에 용량 할당
 */
int vector_reserve(Vector *v, size_t capacity) {
    if (v == NULL) {
        return -1;
    }

    return vector_grow(v, capacity);
}

/**
 * vector_shrink_to_fit - 불필요한 메모리 해제
 */
int vector_shrink_to_fit(Vector *v) {
    if (v == NULL || v->count == 0) {
        return -1;
    }

    if (v->capacity == v->count) {
        return 0;  /* 이미 일치 */
    }

    /* 새로운 메모리 할당 */
    void *new_data = mm_alloc(v->count * v->elem_size);
    if (new_data == NULL) {
        return -1;
    }

    /* 데이터 복사 */
    my_memcpy(new_data, v->data, v->count * v->elem_size);
    mm_free(v->data);

    v->data = new_data;
    v->capacity = v->count;
    return 0;
}

/**
 * vector_find - 요소 검색
 */
int vector_find(Vector *v, const void *elem,
                int (*cmp)(const void*, const void*)) {
    if (v == NULL || elem == NULL || cmp == NULL) {
        return -1;
    }

    for (size_t i = 0; i < v->count; i++) {
        void *current = (char *)v->data + (i * v->elem_size);
        if (cmp(current, elem) == 0) {
            return i;
        }
    }

    return -1;
}

/**
 * vector_foreach - 모든 요소 순회
 */
void vector_foreach(Vector *v,
                   void (*callback)(void*, size_t)) {
    if (v == NULL || callback == NULL) {
        return;
    }

    for (size_t i = 0; i < v->count; i++) {
        void *elem = (char *)v->data + (i * v->elem_size);
        callback(elem, i);
    }
}

/* ============ Debug Functions ============ */

/**
 * vector_dump - 벡터 정보 출력
 */
void vector_dump(Vector *v) {
    if (v == NULL) {
        debug_write("Vector: NULL\n");
        return;
    }

    const char *header = "=== Vector Info ===\n";
    debug_write(header);

    char buf[64];

    debug_write("Element Size: ");
    itoa_simple(v->elem_size, buf, 10);
    debug_write(buf);
    debug_write(" bytes\n");

    debug_write("Count: ");
    itoa_simple(v->count, buf, 10);
    debug_write(buf);
    debug_write("\n");

    debug_write("Capacity: ");
    itoa_simple(v->capacity, buf, 10);
    debug_write(buf);
    debug_write("\n");

    debug_write("Total Size: ");
    itoa_simple(v->capacity * v->elem_size, buf, 10);
    debug_write(buf);
    debug_write(" bytes\n");

    const char *footer = "===================\n";
    debug_write(footer);
}
