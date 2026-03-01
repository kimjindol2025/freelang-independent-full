/**
 * MyOS HashMap (hash.h)
 *
 * 해시 테이블 구현 (Key-Value Store)
 * - Open addressing with linear probing
 * - 동적 리사이징 (load factor 기반)
 * - Generic key-value (void* 기반)
 * - Memory Manager (mm.c) 기반
 *
 * 사용 예시:
 *   HashMap *map = hash_new(sizeof(int), sizeof(char*), hash_int, cmp_int);
 *   int key = 42;
 *   char *value = "hello";
 *   hash_set(map, &key, &value);
 *   char **result = (char**)hash_get(map, &key);
 *   hash_free(map);
 */

#ifndef MYOS_HASH_H
#define MYOS_HASH_H

#include <stddef.h>
#include <stdint.h>

/* ============ Hash Function Type ============ */

/**
 * hash_func_t - 해시 함수 타입
 *
 * 입력: key 포인터, key 크기
 * 출력: 해시 값 (uint32_t)
 */
typedef uint32_t (*hash_func_t)(const void *key, size_t key_size);

/**
 * compare_func_t - 비교 함수 타입
 *
 * 입력: key1, key2 포인터
 * 출력: 0 (같음), 음수 (key1 < key2), 양수 (key1 > key2)
 */
typedef int (*compare_func_t)(const void *key1, const void *key2);

/* ============ HashMap Type ============ */

/**
 * HashMap - 해시 테이블 구조체
 *
 * 내부 필드 (사용자가 직접 접근하면 안 됨):
 * - entries: 엔트리 배열 (key-value 쌍)
 * - capacity: 할당된 슬롯 개수
 * - count: 저장된 엔트리 개수
 * - key_size: 각 키의 크기
 * - value_size: 각 값의 크기
 * - hash_func: 해시 함수
 * - cmp_func: 비교 함수
 */
typedef struct {
    void *entries;              /* HashEntry 배열 */
    size_t capacity;
    size_t count;
    size_t key_size;
    size_t value_size;
    hash_func_t hash_func;
    compare_func_t cmp_func;
} HashMap;

/* ============ HashMap Creation/Destruction ============ */

/**
 * hash_new - 새 해시 테이블 생성
 * @key_size: 키의 크기 (바이트)
 * @value_size: 값의 크기 (바이트)
 * @hash_func: 해시 함수 포인터
 * @cmp_func: 비교 함수 포인터
 *
 * 초기 용량: 16
 * 실패 시 NULL 반환
 */
HashMap* hash_new(size_t key_size, size_t value_size,
                  hash_func_t hash_func, compare_func_t cmp_func);

/**
 * hash_free - 해시 테이블 정리
 * @map: 해시 테이블
 *
 * 모든 메모리 해제 (엔트리 및 구조체)
 * map이 NULL이면 아무 것도 하지 않음
 */
void hash_free(HashMap *map);

/* ============ HashMap Operations ============ */

/**
 * hash_set - 키-값 쌍 저장
 * @map: 해시 테이블
 * @key: 키 포인터
 * @value: 값 포인터
 *
 * 기존 키면 값 업데이트
 * 새 키면 새 엔트리 추가
 * 필요 시 자동으로 용량 확대
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int hash_set(HashMap *map, const void *key, const void *value);

/**
 * hash_get - 키로 값 조회
 * @map: 해시 테이블
 * @key: 키 포인터
 *
 * 반환된 포인터는 값 데이터의 주소
 * 키가 없으면 NULL 반환
 *
 * 예시:
 *   char **result = (char**)hash_get(map, &key);
 */
void* hash_get(HashMap *map, const void *key);

/**
 * hash_delete - 키-값 쌍 삭제
 * @map: 해시 테이블
 * @key: 키 포인터
 * @out_value: 삭제된 값을 저장할 버퍼 (NULL 가능)
 *
 * 키가 있으면 삭제 후 0 반환
 * 키가 없으면 -1 반환
 */
int hash_delete(HashMap *map, const void *key, void *out_value);

/**
 * hash_contains - 키 존재 확인
 * @map: 해시 테이블
 * @key: 키 포인터
 *
 * 반환: 1 (존재) 또는 0 (없음)
 */
int hash_contains(HashMap *map, const void *key);

/**
 * hash_clear - 모든 엔트리 제거
 * @map: 해시 테이블
 *
 * count를 0으로 설정 (capacity는 유지)
 */
void hash_clear(HashMap *map);

/* ============ HashMap Queries ============ */

/**
 * hash_size - 저장된 엔트리 개수
 * @map: 해시 테이블
 */
size_t hash_size(const HashMap *map);

/**
 * hash_capacity - 할당된 슬롯 개수
 * @map: 해시 테이블
 */
size_t hash_capacity(const HashMap *map);

/**
 * hash_is_empty - 해시 테이블이 비어있는지 확인
 * @map: 해시 테이블
 */
int hash_is_empty(const HashMap *map);

/**
 * hash_load_factor - 로드 팩터 계산 (백분율)
 * @map: 해시 테이블
 *
 * 반환: 0 ~ 100
 */
int hash_load_factor(const HashMap *map);

/* ============ HashMap Utilities ============ */

/**
 * hash_resize - 용량 명시적으로 설정
 * @map: 해시 테이블
 * @new_capacity: 새로운 용량
 *
 * 새 용량이 현재 용량보다 작으면 -1 반환
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int hash_resize(HashMap *map, size_t new_capacity);

/**
 * hash_foreach - 모든 엔트리 순회
 * @map: 해시 테이블
 * @callback: 콜백 함수 (void (*)(const void* key, void* value))
 *
 * 각 엔트리에 대해 callback(key, value) 호출
 */
void hash_foreach(HashMap *map,
                 void (*callback)(const void *key, void *value));

/* ============ Built-in Hash Functions ============ */

/**
 * hash_djb2 - DJB2 해시 함수
 *
 * 범용적으로 사용 가능한 해시 함수
 */
uint32_t hash_djb2(const void *key, size_t key_size);

/**
 * hash_int - 32비트 정수 해시
 */
uint32_t hash_int(const void *key, size_t key_size);

/**
 * hash_string - C 문자열 해시
 *
 * key는 char* (문자열 포인터)
 */
uint32_t hash_string(const void *key, size_t key_size);

/* ============ Built-in Compare Functions ============ */

/**
 * cmp_int - 32비트 정수 비교
 */
int cmp_int(const void *key1, const void *key2);

/**
 * cmp_string - C 문자열 비교
 *
 * key는 char* (문자열 포인터)
 */
int cmp_string(const void *key1, const void *key2);

/**
 * cmp_bytes - 바이트 배열 비교
 *
 * 고정 크기 비교 (memcmp)
 */
int cmp_bytes(const void *key1, const void *key2);

/* ============ Debug Functions ============ */

/**
 * hash_dump - 해시 테이블 정보 출력 (syscall 사용)
 * @map: 해시 테이블
 */
void hash_dump(HashMap *map);

/**
 * hash_validate - 해시 테이블 무결성 검증
 * @map: 해시 테이블
 *
 * 반환: 0 (정상) 또는 -1 (오류)
 */
int hash_validate(HashMap *map);

#endif /* MYOS_HASH_H */
