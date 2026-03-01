#include "storage.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

/* MyOS_Lib 통합 */
#include "mm.h"
#include "hash.h"
#include "string.h"

#define INITIAL_CAPACITY 10

/* 해시 함수 */
static uint32_t storage_hash(const void *k, size_t ks) {
    uint32_t h = 5381;
    const uint8_t *b = (const uint8_t *)k;
    for (size_t i = 0; i < ks; i++) h = ((h << 5) + h) + b[i];
    return h;
}

/* 비교 함수 */
static int storage_cmp(const void *k1, const void *k2) {
    return strcmp((const char*)k1, (const char*)k2) == 0 ? 0 : 1;
}

// 새 저장소 생성 (MyOS_Lib HashMap 사용)
Storage *storage_new(void) {
    Storage *s = (Storage*)malloc(sizeof(Storage));
    if (!s) return NULL;

    /* 문자열 키 → 문자열 값 저장 */
    s->items = (StorageItem*)malloc(sizeof(StorageItem) * INITIAL_CAPACITY);
    if (!s->items) {
        free(s);
        return NULL;
    }

    s->count = 0;
    s->capacity = INITIAL_CAPACITY;

    /* MyOS_Lib HashMap 추가 초기화 (선택사항) */
    s->_hash_map = hash_new(128, 512, storage_hash, storage_cmp);

    return s;
}

// 저장소 해제
void storage_free(Storage *s) {
    if (s) {
        if (s->items) free(s->items);
        if (s->_hash_map) hash_free(s->_hash_map);
        free(s);
    }
}

// 저장소 확장
static int storage_resize(Storage *s) {
    int new_capacity = s->capacity * 2;
    StorageItem *new_items = (StorageItem*)malloc(sizeof(StorageItem) * new_capacity);
    if (!new_items) {
        return -1;
    }

    for (int i = 0; i < s->count; i++) {
        new_items[i] = s->items[i];
    }

    free(s->items);
    s->items = new_items;
    s->capacity = new_capacity;
    return 0;
}

// KV 저장 (없으면 생성, 있으면 업데이트)
int storage_set(Storage *s, const char *key, const char *value) {
    if (!s || !key || !value) {
        return -1;
    }

    // 기존 항목 검색
    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->items[i].key, key) == 0) {
            // 업데이트
            strncpy(s->items[i].value, value, sizeof(s->items[i].value) - 1);
            s->items[i].value[sizeof(s->items[i].value) - 1] = '\0';
            s->items[i].updated_at = time(NULL);
            return 0;
        }
    }

    // 용량 확인 및 확장
    if (s->count >= s->capacity) {
        if (storage_resize(s) < 0) {
            return -1;
        }
    }

    // 새 항목 추가
    strncpy(s->items[s->count].key, key, sizeof(s->items[s->count].key) - 1);
    s->items[s->count].key[sizeof(s->items[s->count].key) - 1] = '\0';
    strncpy(s->items[s->count].value, value, sizeof(s->items[s->count].value) - 1);
    s->items[s->count].value[sizeof(s->items[s->count].value) - 1] = '\0';
    s->items[s->count].created_at = time(NULL);
    s->items[s->count].updated_at = time(NULL);

    s->count++;
    return 0;
}

// KV 조회
int storage_get(Storage *s, const char *key, char *out_value, size_t out_size) {
    if (!s || !key || !out_value) {
        return -1;
    }

    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->items[i].key, key) == 0) {
            strncpy(out_value, s->items[i].value, out_size - 1);
            out_value[out_size - 1] = '\0';
            return 0;
        }
    }

    return -1;  // 찾지 못함
}

// KV 삭제
int storage_delete(Storage *s, const char *key) {
    if (!s || !key) {
        return -1;
    }

    for (int i = 0; i < s->count; i++) {
        if (strcmp(s->items[i].key, key) == 0) {
            // 마지막 항목으로 덮어쓰기
            if (i < s->count - 1) {
                s->items[i] = s->items[s->count - 1];
            }
            s->count--;
            return 0;
        }
    }

    return -1;  // 찾지 못함
}

// KV 목록 (JSON 형식)
int storage_list(Storage *s, char *out_json, size_t out_size) {
    if (!s || !out_json || out_size < 10) {
        return -1;
    }

    int written = 0;
    written += snprintf(out_json + written, out_size - written, "[");

    for (int i = 0; i < s->count; i++) {
        if (written > 1) {
            written += snprintf(out_json + written, out_size - written, ",");
        }

        written += snprintf(out_json + written, out_size - written,
                           "{\"key\":\"%s\",\"value\":\"%s\",\"created_at\":%ld,\"updated_at\":%ld}",
                           s->items[i].key, s->items[i].value,
                           s->items[i].created_at, s->items[i].updated_at);

        if (written >= (int)out_size - 20) {
            return -1;  // 버퍼 부족
        }
    }

    written += snprintf(out_json + written, out_size - written, "]");

    return 0;
}
