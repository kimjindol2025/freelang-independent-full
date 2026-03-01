#ifndef MYOS_OBJECT_POOL_H
#define MYOS_OBJECT_POOL_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Object Pool - 객체 할당 및 재사용 관리
 * ============================================================================ */

typedef struct {
    void **objects;             /* 풀에 저장된 객체 배열 */
    uint8_t *allocated;         /* 각 슬롯이 할당되었는지 표시 (1=할당, 0=미할당) */
    size_t capacity;            /* 풀의 전체 용량 */
    size_t count;               /* 현재 할당된 객체 수 */
} ObjectPool;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Object Pool 생성
 * @param capacity: 최대 객체 개수
 */
ObjectPool* pool_new(size_t capacity);

/**
 * 풀에서 객체 할당
 * @param pool: Object Pool
 * @param obj: 할당할 객체 포인터
 * @param out_id: 할당된 ID (반환값)
 * @return: 0 (성공), -1 (실패)
 */
int pool_allocate(ObjectPool *pool, void *obj, size_t *out_id);

/**
 * 풀에서 객체 조회
 * @param pool: Object Pool
 * @param id: 객체 ID
 * @return: 객체 포인터 또는 NULL
 */
void* pool_get(ObjectPool *pool, size_t id);

/**
 * 풀에서 객체 해제 (재사용 가능하게 표시)
 * @param pool: Object Pool
 * @param id: 객체 ID
 * @return: 0 (성공), -1 (실패)
 */
int pool_release(ObjectPool *pool, size_t id);

/**
 * 풀의 할당 상태 확인
 * @param pool: Object Pool
 * @param id: 객체 ID
 * @return: 1 (할당됨), 0 (미할당), -1 (유효하지 않은 ID)
 */
int pool_is_allocated(ObjectPool *pool, size_t id);

/**
 * 풀의 현재 사용률 조회
 */
size_t pool_count(ObjectPool *pool);

/**
 * Object Pool 해제
 */
void pool_free(ObjectPool *pool);

#endif
