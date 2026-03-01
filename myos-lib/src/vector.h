/**
 * MyOS Vector (vector.h)
 *
 * 동적 배열 구현
 * - Generic 컨테이너 (void* 기반)
 * - 자동 리사이징 (capacity 관리)
 * - push/pop/at 기능
 * - Memory Manager (mm.c) 기반
 *
 * 사용 예시:
 *   Vector *v = vector_new(sizeof(int));
 *   int x = 42;
 *   vector_push(v, &x);
 *   int *elem = (int*)vector_at(v, 0);
 *   vector_free(v);
 */

#ifndef MYOS_VECTOR_H
#define MYOS_VECTOR_H

#include <stddef.h>
#include <stdint.h>

/* ============ Vector Type ============ */

/**
 * Vector - 동적 배열 구조체
 *
 * 내부 필드 (사용자가 직접 접근하면 안 됨):
 * - data: 요소 저장소 (void *배열)
 * - count: 현재 요소 개수
 * - capacity: 할당된 용량
 * - elem_size: 각 요소의 크기
 */
typedef struct {
    void *data;
    size_t count;
    size_t capacity;
    size_t elem_size;
} Vector;

/* ============ Vector Creation/Destruction ============ */

/**
 * vector_new - 새 벡터 생성
 * @elem_size: 각 요소의 크기 (바이트)
 *
 * 초기 용량: 16개 요소
 * 실패 시 NULL 반환
 *
 * 예시:
 *   Vector *ints = vector_new(sizeof(int));
 *   Vector *strs = vector_new(sizeof(char*));
 */
Vector* vector_new(size_t elem_size);

/**
 * vector_free - 벡터 정리
 * @v: 벡터 포인터
 *
 * 모든 메모리 해제 (요소 및 구조체)
 * v가 NULL이면 아무 것도 하지 않음
 */
void vector_free(Vector *v);

/* ============ Vector Operations ============ */

/**
 * vector_push - 요소 추가
 * @v: 벡터
 * @elem: 추가할 요소의 포인터
 *
 * 필요 시 자동으로 용량 확대 (2배)
 * 반환: 0 (성공) 또는 -1 (실패)
 *
 * 예시:
 *   int x = 42;
 *   vector_push(v, &x);
 */
int vector_push(Vector *v, const void *elem);

/**
 * vector_pop - 마지막 요소 제거
 * @v: 벡터
 * @out: 제거된 요소를 저장할 버퍼 (NULL 가능)
 *
 * count가 0이면 아무 것도 하지 않음
 * out이 NULL이 아니면 제거된 요소를 out에 복사
 *
 * 예시:
 *   int x;
 *   vector_pop(v, &x);  // x에 마지막 요소 저장
 */
void vector_pop(Vector *v, void *out);

/**
 * vector_at - 인덱스로 요소 접근
 * @v: 벡터
 * @index: 인덱스 (0 부터 시작)
 *
 * 범위를 벗어나면 NULL 반환
 * 반환된 포인터는 요소 데이터의 주소
 *
 * 예시:
 *   int *elem = (int*)vector_at(v, 0);
 */
void* vector_at(Vector *v, size_t index);

/**
 * vector_set - 인덱스의 요소 설정
 * @v: 벡터
 * @index: 인덱스
 * @elem: 설정할 요소의 포인터
 *
 * 범위를 벗어나면 -1 반환
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int vector_set(Vector *v, size_t index, const void *elem);

/**
 * vector_insert - 특정 인덱스에 요소 삽입
 * @v: 벡터
 * @index: 삽입 위치 (0 부터 count까지 가능)
 * @elem: 삽입할 요소의 포인터
 *
 * 기존 요소들을 뒤로 밀어냄
 * index가 범위를 벗어나면 -1 반환
 */
int vector_insert(Vector *v, size_t index, const void *elem);

/**
 * vector_remove - 특정 인덱스 요소 제거
 * @v: 벡터
 * @index: 제거할 인덱스
 * @out: 제거된 요소를 저장할 버퍼 (NULL 가능)
 *
 * 뒤의 요소들을 앞으로 당겨냄
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int vector_remove(Vector *v, size_t index, void *out);

/**
 * vector_clear - 모든 요소 제거
 * @v: 벡터
 *
 * count를 0으로 설정 (capacity는 유지)
 */
void vector_clear(Vector *v);

/* ============ Vector Queries ============ */

/**
 * vector_size - 현재 요소 개수 조회
 * @v: 벡터
 */
size_t vector_size(const Vector *v);

/**
 * vector_capacity - 할당된 용량 조회
 * @v: 벡터
 */
size_t vector_capacity(const Vector *v);

/**
 * vector_is_empty - 벡터가 비어있는지 확인
 * @v: 벡터
 */
int vector_is_empty(const Vector *v);

/**
 * vector_get_elem_size - 요소 크기 조회
 * @v: 벡터
 */
size_t vector_get_elem_size(const Vector *v);

/* ============ Vector Utilities ============ */

/**
 * vector_reserve - 사전에 용량 할당
 * @v: 벡터
 * @capacity: 원하는 용량
 *
 * 현재 용량보다 작으면 아무 것도 하지 않음
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int vector_reserve(Vector *v, size_t capacity);

/**
 * vector_shrink_to_fit - 불필요한 메모리 해제
 * @v: 벡터
 *
 * capacity를 count로 줄임
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int vector_shrink_to_fit(Vector *v);

/**
 * vector_find - 요소 검색
 * @v: 벡터
 * @elem: 찾을 요소의 포인터
 * @cmp: 비교 함수 (int (*)(const void*, const void*))
 *
 * cmp가 0을 반환하면 일치
 * 찾으면 인덱스 반환, 못 찾으면 -1 반환
 */
int vector_find(Vector *v, const void *elem,
                int (*cmp)(const void*, const void*));

/**
 * vector_foreach - 모든 요소 순회
 * @v: 벡터
 * @callback: 콜백 함수 (void (*)(void*, size_t))
 *
 * 각 요소에 대해 callback(elem, index) 호출
 */
void vector_foreach(Vector *v,
                   void (*callback)(void*, size_t));

/* ============ Debug Functions ============ */

/**
 * vector_dump - 벡터 정보 출력 (syscall 사용)
 * @v: 벡터
 */
void vector_dump(Vector *v);

#endif /* MYOS_VECTOR_H */
