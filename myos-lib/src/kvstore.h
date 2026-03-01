#ifndef MYOS_KVSTORE_H
#define MYOS_KVSTORE_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * KV Store - 파일 기반 키-값 저장소 (Zero-Dependency)
 * ============================================================================ */

typedef struct {
    int fd;                  /* 파일 디스크립터 */
    char *path;             /* 파일 경로 */
    uint32_t entry_count;   /* 항목 수 */
} KVStore;

/* KVStore 열기/생성 */
KVStore* kvstore_open(const char *path);

/* KVStore 닫기 */
void kvstore_close(KVStore *store);

/* 키-값 저장 */
int kvstore_put(KVStore *store, const char *key, const char *value);

/* 키로 값 조회 */
int kvstore_get(KVStore *store, const char *key, char *value, size_t value_size);

/* 키 삭제 */
int kvstore_delete(KVStore *store, const char *key);

/* 모든 항목 나열 */
int kvstore_iterate(KVStore *store,
                    int (*callback)(const char *key, const char *value, void *ctx),
                    void *ctx);

/* 저장소 크기 조회 */
uint32_t kvstore_size(KVStore *store);

#endif
