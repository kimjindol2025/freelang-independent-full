#include "storage.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define INITIAL_CAPACITY 10

// 새 저장소 생성
Storage *storage_new(void) {
    Storage *s = (Storage*)malloc(sizeof(Storage));
    if (!s) return NULL;

    s->items = (StorageItem*)malloc(sizeof(StorageItem) * INITIAL_CAPACITY);
    if (!s->items) {
        free(s);
        return NULL;
    }

    s->count = 0;
    s->capacity = INITIAL_CAPACITY;
    return s;
}

// 저장소 해제
void storage_free(Storage *s) {
    if (s) {
        if (s->items) free(s->items);
        free(s);
    }
}

// 저장소 확장
static int storage_resize(Storage *s) {
    int new_capacity = s->capacity * 2;
    StorageItem *new_items = (StorageItem*)realloc(s->items,
                                                     sizeof(StorageItem) * new_capacity);
    if (!new_items) {
        return -1;
    }

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
    strncpy(s->items[s->count].value, value, sizeof(s->items[s->count].value) - 1);
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
                memmove(&s->items[i], &s->items[s->count - 1], sizeof(StorageItem));
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
                           "{\"key\":\"%s\",\"value\":\"%s\"}",
                           s->items[i].key, s->items[i].value);

        if (written >= (int)out_size - 10) {
            return -1;  // 버퍼 부족
        }
    }

    written += snprintf(out_json + written, out_size - written, "]");

    return 0;
}
