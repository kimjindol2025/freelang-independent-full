#ifndef STORAGE_H
#define STORAGE_H

#include <stddef.h>

// KV 저장소 아이템
typedef struct StorageItem {
    char key[128];
    char value[512];
    long created_at;
    long updated_at;
} StorageItem;

// KV 저장소 (간단한 해시맵)
typedef struct {
    StorageItem *items;
    int count;
    int capacity;
} Storage;

// 함수
Storage *storage_new(void);
void storage_free(Storage *s);
int storage_set(Storage *s, const char *key, const char *value);
int storage_get(Storage *s, const char *key, char *out_value, size_t out_size);
int storage_delete(Storage *s, const char *key);
int storage_list(Storage *s, char *out_json, size_t out_size);

#endif
