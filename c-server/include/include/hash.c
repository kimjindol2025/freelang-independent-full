/**
 * MyOS HashMap (hash.c)
 *
 * 해시 테이블 구현 (Open Addressing with Linear Probing)
 * - Memory Manager (mm.c) 기반
 * - 동적 리사이징 (load factor 기반)
 * - Generic key-value 저장
 */

#include "hash.h"
#include "mm.h"
#include <sys/syscall.h>

/* ============ Internal Data Structures ============ */

/**
 * HashEntry - 해시 테이블 엔트리
 *
 * Open addressing 방식:
 * - is_occupied: 1 (사용중), 0 (빈 슬롯), -1 (삭제됨)
 */
typedef struct {
    void *key;
    void *value;
    uint32_t hash;
    int is_occupied;  /* 1: occupied, 0: empty, -1: deleted */
} HashEntry;

/* ============ Internal Utilities ============ */

/**
 * memmove - 메모리 이동
 */
static void* my_memmove(void *dest, const void *src, size_t n) {
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
 * memcpy - 메모리 복사
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

/* ============ Built-in Hash Functions ============ */

/**
 * hash_djb2 - DJB2 알고리즘
 *
 * 범용적으로 좋은 성능의 해시 함수
 */
uint32_t hash_djb2(const void *key, size_t key_size) {
    uint32_t hash = 5381;
    const unsigned char *bytes = (const unsigned char *)key;

    for (size_t i = 0; i < key_size; i++) {
        hash = ((hash << 5) + hash) + bytes[i];  /* hash * 33 + byte */
    }

    return hash;
}

/**
 * hash_int - 정수 해시
 */
uint32_t hash_int(const void *key, size_t key_size) {
    (void)key_size;  /* 경고 제거 */
    uint32_t x = *(const uint32_t *)key;
    /* Knuth's multiplicative hash */
    return (x * 2654435761U) >> 16;
}

/**
 * hash_string - 문자열 해시
 */
uint32_t hash_string(const void *key, size_t key_size) {
    (void)key_size;  /* 경고 제거 */

    const char *str = *(const char * const *)key;
    if (str == NULL) return 0;

    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

/* ============ Built-in Compare Functions ============ */

/**
 * cmp_int - 정수 비교
 */
int cmp_int(const void *key1, const void *key2) {
    int v1 = *(const int *)key1;
    int v2 = *(const int *)key2;
    if (v1 < v2) return -1;
    if (v1 > v2) return 1;
    return 0;
}

/**
 * cmp_string - 문자열 비교
 */
int cmp_string(const void *key1, const void *key2) {
    const char *str1 = *(const char * const *)key1;
    const char *str2 = *(const char * const *)key2;

    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;

    while (*str1 && *str1 == *str2) {
        str1++;
        str2++;
    }

    if (*str1 < *str2) return -1;
    if (*str1 > *str2) return 1;
    return 0;
}

/**
 * cmp_bytes - 바이트 배열 비교
 */
int cmp_bytes(const void *key1, const void *key2) {
    const unsigned char *b1 = (const unsigned char *)key1;
    const unsigned char *b2 = (const unsigned char *)key2;

    for (size_t i = 0; i < 32; i++) {  /* 임시 크기 */
        if (b1[i] < b2[i]) return -1;
        if (b1[i] > b2[i]) return 1;
        if (b1[i] == 0) break;
    }
    return 0;
}

/* ============ HashMap Internal ============ */

/**
 * hash_find_entry - 엔트리 찾기 (Open Addressing)
 *
 * 충돌 시 다음 슬롯 탐사 (linear probing)
 * 반환: 엔트리 인덱스 또는 -1 (없음)
 */
static int hash_find_entry(HashMap *map, const void *key,
                          uint32_t hash, int find_empty) {
    size_t capacity = map->capacity;
    size_t index = hash % capacity;
    size_t start = index;

    while (1) {
        HashEntry *entry = (HashEntry *)map->entries + index;

        /* 빈 슬롯 찾음 */
        if (entry->is_occupied == 0) {
            if (find_empty) return index;
            return -1;  /* 키 없음 */
        }

        /* 삭제된 슬롯은 스킵 */
        if (entry->is_occupied == -1) {
            if (find_empty) return index;
            index = (index + 1) % capacity;
            if (index == start) return -1;  /* 테이블 가득참 */
            continue;
        }

        /* 해시 및 키 일치 확인 */
        if (entry->hash == hash && map->cmp_func(entry->key, key) == 0) {
            return index;
        }

        /* 다음 슬롯으로 */
        index = (index + 1) % capacity;
        if (index == start) return -1;  /* 테이블 가득참 */
    }
}

/**
 * hash_grow - 테이블 확장
 *
 * 새로운 용량: max(current * 2, 16)
 */
static int hash_grow(HashMap *map, size_t min_capacity) {
    if (map->capacity >= min_capacity) {
        return 0;  /* 이미 충분함 */
    }

    /* 새로운 용량 계산 */
    size_t new_capacity = map->capacity * 2;
    if (new_capacity < 16) new_capacity = 16;
    if (new_capacity < min_capacity) new_capacity = min_capacity;

    /* 새 엔트리 배열 할당 */
    void *new_entries = mm_alloc(new_capacity * sizeof(HashEntry));
    if (new_entries == NULL) {
        return -1;
    }

    /* 새 배열 초기화 */
    for (size_t i = 0; i < new_capacity; i++) {
        HashEntry *entry = (HashEntry *)new_entries + i;
        entry->is_occupied = 0;
        entry->key = NULL;
        entry->value = NULL;
        entry->hash = 0;
    }

    /* 기존 엔트리 재해시 */
    void *old_entries = map->entries;
    size_t old_capacity = map->capacity;

    map->entries = new_entries;
    map->capacity = new_capacity;
    map->count = 0;  /* 재계산 */

    /* 모든 엔트리 재삽입 */
    for (size_t i = 0; i < old_capacity; i++) {
        HashEntry *old_entry = (HashEntry *)old_entries + i;

        if (old_entry->is_occupied == 1) {
            /* 재해시 */
            int idx = hash_find_entry(map, old_entry->key,
                                     old_entry->hash, 1);
            if (idx >= 0) {
                HashEntry *new_entry = (HashEntry *)map->entries + idx;
                new_entry->key = old_entry->key;
                new_entry->value = old_entry->value;
                new_entry->hash = old_entry->hash;
                new_entry->is_occupied = 1;
                map->count++;
            }
        }
    }

    /* 기존 메모리 해제 */
    if (old_entries != NULL) {
        mm_free(old_entries);
    }

    return 0;
}

/* ============ HashMap API Implementation ============ */

/**
 * hash_new - 새 해시 테이블 생성
 */
HashMap* hash_new(size_t key_size, size_t value_size,
                  hash_func_t hash_func, compare_func_t cmp_func) {
    if (key_size == 0 || value_size == 0 || hash_func == NULL ||
        cmp_func == NULL) {
        return NULL;
    }

    /* HashMap 구조체 할당 */
    HashMap *map = (HashMap *)mm_alloc(sizeof(HashMap));
    if (map == NULL) {
        return NULL;
    }

    /* 초기 엔트리 배열 할당 (16 슬롯) */
    size_t initial_capacity = 16;
    void *entries = mm_alloc(initial_capacity * sizeof(HashEntry));
    if (entries == NULL) {
        mm_free(map);
        return NULL;
    }

    /* 엔트리 초기화 */
    for (size_t i = 0; i < initial_capacity; i++) {
        HashEntry *entry = (HashEntry *)entries + i;
        entry->is_occupied = 0;
        entry->key = NULL;
        entry->value = NULL;
        entry->hash = 0;
    }

    map->entries = entries;
    map->capacity = initial_capacity;
    map->count = 0;
    map->key_size = key_size;
    map->value_size = value_size;
    map->hash_func = hash_func;
    map->cmp_func = cmp_func;

    return map;
}

/**
 * hash_free - 해시 테이블 정리
 */
void hash_free(HashMap *map) {
    if (map == NULL) {
        return;
    }

    if (map->entries != NULL) {
        /* 모든 키, 값 해제 */
        for (size_t i = 0; i < map->capacity; i++) {
            HashEntry *entry = (HashEntry *)map->entries + i;
            if (entry->is_occupied == 1) {
                if (entry->key != NULL) mm_free(entry->key);
                if (entry->value != NULL) mm_free(entry->value);
            }
        }
        mm_free(map->entries);
    }

    mm_free(map);
}

/**
 * hash_set - 키-값 쌍 저장
 */
int hash_set(HashMap *map, const void *key, const void *value) {
    if (map == NULL || key == NULL || value == NULL) {
        return -1;
    }

    /* 로드 팩터 확인: count / capacity > 0.75 */
    if (map->count * 4 > map->capacity * 3) {
        if (hash_grow(map, map->capacity * 2) < 0) {
            return -1;
        }
    }

    /* 해시 계산 */
    uint32_t hash = map->hash_func(key, map->key_size);

    /* 엔트리 찾기 */
    int idx = hash_find_entry(map, key, hash, 1);
    if (idx < 0) {
        return -1;  /* 테이블 가득참 */
    }

    HashEntry *entry = (HashEntry *)map->entries + idx;

    /* 새 키 저장 */
    if (entry->is_occupied != 1) {
        /* 새 엔트리 */
        void *new_key = mm_alloc(map->key_size);
        if (new_key == NULL) {
            return -1;
        }
        my_memcpy(new_key, key, map->key_size);
        entry->key = new_key;
        map->count++;
    }

    /* 값 저장 */
    if (entry->value == NULL) {
        entry->value = mm_alloc(map->value_size);
        if (entry->value == NULL) {
            return -1;
        }
    }

    my_memcpy(entry->value, value, map->value_size);
    entry->hash = hash;
    entry->is_occupied = 1;

    return 0;
}

/**
 * hash_get - 키로 값 조회
 */
void* hash_get(HashMap *map, const void *key) {
    if (map == NULL || key == NULL) {
        return NULL;
    }

    uint32_t hash = map->hash_func(key, map->key_size);
    int idx = hash_find_entry(map, key, hash, 0);

    if (idx < 0) {
        return NULL;
    }

    HashEntry *entry = (HashEntry *)map->entries + idx;
    return entry->value;
}

/**
 * hash_delete - 키-값 쌍 삭제
 */
int hash_delete(HashMap *map, const void *key, void *out_value) {
    if (map == NULL || key == NULL) {
        return -1;
    }

    uint32_t hash = map->hash_func(key, map->key_size);
    int idx = hash_find_entry(map, key, hash, 0);

    if (idx < 0) {
        return -1;
    }

    HashEntry *entry = (HashEntry *)map->entries + idx;

    /* 값 복사 */
    if (out_value != NULL) {
        my_memcpy(out_value, entry->value, map->value_size);
    }

    /* 엔트리 정리 */
    if (entry->key != NULL) {
        mm_free(entry->key);
        entry->key = NULL;
    }
    if (entry->value != NULL) {
        mm_free(entry->value);
        entry->value = NULL;
    }

    entry->is_occupied = -1;  /* 삭제 표시 */
    map->count--;

    return 0;
}

/**
 * hash_contains - 키 존재 확인
 */
int hash_contains(HashMap *map, const void *key) {
    if (map == NULL || key == NULL) {
        return 0;
    }

    uint32_t hash = map->hash_func(key, map->key_size);
    int idx = hash_find_entry(map, key, hash, 0);

    return (idx >= 0) ? 1 : 0;
}

/**
 * hash_clear - 모든 엔트리 제거
 */
void hash_clear(HashMap *map) {
    if (map == NULL) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        HashEntry *entry = (HashEntry *)map->entries + i;
        if (entry->is_occupied == 1) {
            if (entry->key != NULL) {
                mm_free(entry->key);
                entry->key = NULL;
            }
            if (entry->value != NULL) {
                mm_free(entry->value);
                entry->value = NULL;
            }
        }
        entry->is_occupied = 0;
    }

    map->count = 0;
}

/* ============ HashMap Queries ============ */

/**
 * hash_size - 저장된 엔트리 개수
 */
size_t hash_size(const HashMap *map) {
    return (map != NULL) ? map->count : 0;
}

/**
 * hash_capacity - 할당된 슬롯 개수
 */
size_t hash_capacity(const HashMap *map) {
    return (map != NULL) ? map->capacity : 0;
}

/**
 * hash_is_empty - 비어있는지 확인
 */
int hash_is_empty(const HashMap *map) {
    return (map == NULL) || (map->count == 0);
}

/**
 * hash_load_factor - 로드 팩터 (백분율)
 */
int hash_load_factor(const HashMap *map) {
    if (map == NULL || map->capacity == 0) {
        return 0;
    }
    return (int)((map->count * 100) / map->capacity);
}

/* ============ HashMap Utilities ============ */

/**
 * hash_resize - 용량 명시적으로 설정
 */
int hash_resize(HashMap *map, size_t new_capacity) {
    if (map == NULL || new_capacity < map->count) {
        return -1;
    }

    return hash_grow(map, new_capacity);
}

/**
 * hash_foreach - 모든 엔트리 순회
 */
void hash_foreach(HashMap *map,
                 void (*callback)(const void *key, void *value)) {
    if (map == NULL || callback == NULL) {
        return;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        HashEntry *entry = (HashEntry *)map->entries + i;
        if (entry->is_occupied == 1) {
            callback(entry->key, entry->value);
        }
    }
}

/* ============ Debug Functions ============ */

/**
 * hash_dump - 해시 테이블 정보 출력
 */
void hash_dump(HashMap *map) {
    if (map == NULL) {
        debug_write("HashMap: NULL\n");
        return;
    }

    const char *header = "=== HashMap Info ===\n";
    debug_write(header);

    char buf[64];

    debug_write("Capacity: ");
    itoa_simple(map->capacity, buf, 10);
    debug_write(buf);
    debug_write("\n");

    debug_write("Count: ");
    itoa_simple(map->count, buf, 10);
    debug_write(buf);
    debug_write("\n");

    debug_write("Load Factor: ");
    itoa_simple(hash_load_factor(map), buf, 10);
    debug_write(buf);
    debug_write("%\n");

    debug_write("Key Size: ");
    itoa_simple(map->key_size, buf, 10);
    debug_write(buf);
    debug_write(" bytes\n");

    debug_write("Value Size: ");
    itoa_simple(map->value_size, buf, 10);
    debug_write(buf);
    debug_write(" bytes\n");

    const char *footer = "===================\n";
    debug_write(footer);
}

/**
 * hash_validate - 해시 테이블 무결성 검증
 */
int hash_validate(HashMap *map) {
    if (map == NULL) {
        return -1;
    }

    size_t count = 0;

    for (size_t i = 0; i < map->capacity; i++) {
        HashEntry *entry = (HashEntry *)map->entries + i;

        if (entry->is_occupied == 1) {
            /* 엔트리 검증 */
            if (entry->key == NULL || entry->value == NULL) {
                debug_write("hash_validate: NULL key or value\n");
                return -1;
            }
            count++;
        }
    }

    /* 카운트 검증 */
    if (count != map->count) {
        debug_write("hash_validate: count mismatch\n");
        return -1;
    }

    return 0;
}
