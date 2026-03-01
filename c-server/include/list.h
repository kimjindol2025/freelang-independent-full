/**
 * MyOS_Lib Linked List (Phase B-4)
 *
 * 동적 연결 리스트 구현
 * - Memory Manager 기반
 * - Generic void* 타입 저장
 * - Iterator 지원
 * - Zero-Dependency
 */

#ifndef MYOS_LIST_H
#define MYOS_LIST_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Linked List 타입 정의
 * ============================================================================ */

/**
 * ListNode - 연결 리스트 노드
 */
typedef struct ListNode {
    void *data;              /* 저장된 데이터 포인터 */
    struct ListNode *next;   /* 다음 노드 */
    struct ListNode *prev;   /* 이전 노드 (양방향) */
} ListNode;

/**
 * LinkedList - 연결 리스트 구조체
 */
typedef struct {
    ListNode *head;          /* 첫 번째 노드 */
    ListNode *tail;          /* 마지막 노드 */
    size_t count;            /* 요소 개수 */
    size_t elem_size;        /* 각 요소의 크기 (바이트) */
} LinkedList;

/**
 * Iterator - 리스트 순회용 반복자
 */
typedef struct {
    LinkedList *list;        /* 소속 리스트 */
    ListNode *current;       /* 현재 노드 */
    size_t index;            /* 현재 인덱스 */
} ListIterator;

/* ============================================================================
 * 생성/소멸
 * ============================================================================ */

/**
 * 새 연결 리스트 생성
 * @param elem_size 각 요소의 크기 (바이트)
 * @return 새 LinkedList 또는 NULL (실패)
 */
LinkedList* list_new(size_t elem_size);

/**
 * 연결 리스트 해제
 * @param list 해제할 리스트
 */
void list_free(LinkedList *list);

/* ============================================================================
 * 데이터 추가/제거
 * ============================================================================ */

/**
 * 앞에 요소 추가
 * @param list 대상 리스트
 * @param elem 추가할 요소
 * @return 0 성공, -1 실패
 */
int list_push_front(LinkedList *list, const void *elem);

/**
 * 뒤에 요소 추가
 * @param list 대상 리스트
 * @param elem 추가할 요소
 * @return 0 성공, -1 실패
 */
int list_push_back(LinkedList *list, const void *elem);

/**
 * 앞 요소 제거
 * @param list 대상 리스트
 * @param out 제거된 요소 저장소 (NULL 가능)
 * @return 0 성공, -1 실패 (빈 리스트)
 */
int list_pop_front(LinkedList *list, void *out);

/**
 * 뒤 요소 제거
 * @param list 대상 리스트
 * @param out 제거된 요소 저장소 (NULL 가능)
 * @return 0 성공, -1 실패 (빈 리스트)
 */
int list_pop_back(LinkedList *list, void *out);

/**
 * 특정 위치에 요소 삽입
 * @param list 대상 리스트
 * @param index 삽입 위치 (0부터 시작)
 * @param elem 삽입할 요소
 * @return 0 성공, -1 실패
 */
int list_insert(LinkedList *list, size_t index, const void *elem);

/**
 * 특정 위치의 요소 제거
 * @param list 대상 리스트
 * @param index 제거할 위치
 * @param out 제거된 요소 저장소 (NULL 가능)
 * @return 0 성공, -1 실패
 */
int list_remove(LinkedList *list, size_t index, void *out);

/**
 * 모든 요소 제거
 * @param list 대상 리스트
 */
void list_clear(LinkedList *list);

/* ============================================================================
 * 쿼리/접근
 * ============================================================================ */

/**
 * 리스트 크기
 * @param list 대상 리스트
 * @return 요소 개수
 */
size_t list_size(const LinkedList *list);

/**
 * 리스트가 비어있는지 확인
 * @param list 대상 리스트
 * @return 1 비어있음, 0 아님
 */
int list_is_empty(const LinkedList *list);

/**
 * 특정 위치의 요소 조회
 * @param list 대상 리스트
 * @param index 위치 (0부터 시작)
 * @return 요소 포인터 또는 NULL (범위 초과)
 */
void* list_at(LinkedList *list, size_t index);

/**
 * 특정 위치의 요소 설정
 * @param list 대상 리스트
 * @param index 위치
 * @param elem 새 요소
 * @return 0 성공, -1 실패
 */
int list_set(LinkedList *list, size_t index, const void *elem);

/**
 * 앞 요소 조회 (제거 없음)
 * @param list 대상 리스트
 * @return 요소 포인터 또는 NULL
 */
void* list_front(LinkedList *list);

/**
 * 뒤 요소 조회 (제거 없음)
 * @param list 대상 리스트
 * @return 요소 포인터 또는 NULL
 */
void* list_back(LinkedList *list);

/* ============================================================================
 * 검색/비교
 * ============================================================================ */

/**
 * 요소 찾기
 * @param list 대상 리스트
 * @param elem 찾을 요소
 * @param cmp 비교 함수 (0 같음, <0 elem1<elem2, >0 elem1>elem2)
 * @return 위치 또는 -1 (없음)
 */
int list_find(LinkedList *list, const void *elem,
              int (*cmp)(const void*, const void*));

/**
 * 조건을 만족하는 첫 번째 요소 찾기
 * @param list 대상 리스트
 * @param predicate 조건 함수 (1 true, 0 false)
 * @return 위치 또는 -1 (없음)
 */
int list_find_if(LinkedList *list,
                 int (*predicate)(const void*));

/* ============================================================================
 * 순회
 * ============================================================================ */

/**
 * 모든 요소에 대해 함수 실행
 * @param list 대상 리스트
 * @param callback 실행할 함수
 */
void list_foreach(LinkedList *list,
                  void (*callback)(void*, size_t));

/**
 * 역순으로 모든 요소에 대해 함수 실행
 * @param list 대상 리스트
 * @param callback 실행할 함수
 */
void list_foreach_reverse(LinkedList *list,
                          void (*callback)(void*, size_t));

/* ============================================================================
 * Iterator (고급)
 * ============================================================================ */

/**
 * Iterator 생성 (처음부터)
 * @param list 대상 리스트
 * @return 새 Iterator
 */
ListIterator list_iterator(LinkedList *list);

/**
 * Iterator가 유효한지 확인
 * @param it Iterator
 * @return 1 유효, 0 끝
 */
int list_iterator_valid(const ListIterator *it);

/**
 * Iterator 다음으로 이동
 * @param it Iterator
 * @return 1 성공, 0 끝
 */
int list_iterator_next(ListIterator *it);

/**
 * Iterator 현재 요소 조회
 * @param it Iterator
 * @return 요소 포인터
 */
void* list_iterator_value(const ListIterator *it);

/**
 * Iterator 현재 인덱스
 * @param it Iterator
 * @return 인덱스 (0부터)
 */
size_t list_iterator_index(const ListIterator *it);

/* ============================================================================
 * 고급 기능
 * ============================================================================ */

/**
 * 리스트 역순 정렬
 * @param list 대상 리스트
 */
void list_reverse(LinkedList *list);

/**
 * 요소 개수로 제거 (첫 번째부터)
 * @param list 대상 리스트
 * @param count 제거할 개수
 * @return 실제 제거된 개수
 */
size_t list_erase_first_n(LinkedList *list, size_t count);

/**
 * 조건을 만족하는 모든 요소 제거
 * @param list 대상 리스트
 * @param predicate 조건 함수
 * @return 제거된 개수
 */
size_t list_remove_if(LinkedList *list,
                      int (*predicate)(const void*));

/**
 * 중복 제거
 * @param list 대상 리스트
 * @param cmp 비교 함수
 * @return 제거된 개수
 */
size_t list_unique(LinkedList *list,
                   int (*cmp)(const void*, const void*));

/**
 * 두 리스트 합치기 (list2는 비워짐)
 * @param list1 대상 리스트
 * @param list2 병합할 리스트
 * @return 0 성공, -1 실패
 */
int list_merge(LinkedList *list1, LinkedList *list2);

/* ============================================================================
 * 디버그
 * ============================================================================ */

/**
 * 리스트 정보 출력
 * @param list 대상 리스트
 * @param label 레이블
 */
void list_dump(const LinkedList *list, const char *label);

/**
 * 리스트 무결성 검증
 * @param list 대상 리스트
 * @return 0 유효, -1 손상
 */
int list_validate(const LinkedList *list);

#endif /* MYOS_LIST_H */
