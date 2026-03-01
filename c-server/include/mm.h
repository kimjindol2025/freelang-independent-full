/**
 * MyOS Memory Manager (mm.h)
 *
 * Zero-Dependency 메모리 관리자
 * - syscall 직접 호출 (mmap, munmap)
 * - libc malloc/free 제거
 * - Free-list 기반 동적 할당
 * - 통계 추적
 *
 * API:
 *   - mm_init(size): 힙 초기화
 *   - mm_alloc(size): 메모리 할당
 *   - mm_free(ptr): 메모리 해제
 *   - mm_get_stats(): 통계 조회
 *   - mm_destroy(): 정리
 */

#ifndef MYOS_MM_H
#define MYOS_MM_H

#include <stddef.h>
#include <stdint.h>

/* ============ Memory Manager 초기화 ============ */

/**
 * mm_init - 힙 메모리 초기화
 * @heap_size: 할당할 힙 크기 (바이트)
 *
 * mmap()을 사용해 가상 메모리 할당
 * 실패 시 -1 반환
 */
int mm_init(size_t heap_size);

/* ============ Memory Allocation/Deallocation ============ */

/**
 * mm_alloc - 메모리 블록 할당
 * @size: 요청 크기 (바이트)
 *
 * Free-list에서 적절한 크기의 블록 탐색 (first-fit)
 * 블록이 없으면 새로 할당
 * 할당 실패 시 NULL 반환
 *
 * 할당된 메모리는 16바이트 정렬됨 (캐시 효율성)
 */
void* mm_alloc(size_t size);

/**
 * mm_free - 메모리 블록 해제
 * @ptr: 할당된 블록의 포인터
 *
 * 블록을 free-list로 반환
 * 인접한 해제 블록과 병합 (coalescing)
 * ptr이 NULL이거나 잘못된 주소면 아무 것도 하지 않음
 */
void mm_free(void *ptr);

/* ============ Memory Statistics ============ */

typedef struct {
    uint64_t total_heap_size;    /* 전체 힙 크기 */
    uint64_t allocated_size;     /* 현재 할당된 크기 */
    uint64_t free_size;          /* 현재 사용 가능한 크기 */
    uint64_t total_allocations;  /* 누적 할당 횟수 */
    uint64_t total_deallocations;/* 누적 해제 횟수 */
    uint32_t free_blocks_count;  /* Free-list 블록 개수 */
    uint32_t allocated_blocks_count; /* 할당된 블록 개수 */
} MMStats;

/**
 * mm_get_stats - 메모리 통계 조회
 * @out: 통계 정보를 저장할 구조체 포인터
 *
 * 현재 메모리 상태를 out에 저장
 * 실패 시 -1 반환
 */
int mm_get_stats(MMStats *out);

/* ============ Memory Manager Cleanup ============ */

/**
 * mm_destroy - 메모리 매니저 정리
 *
 * mmap으로 할당한 메모리를 munmap으로 해제
 * 더 이상 mm_alloc/mm_free를 사용할 수 없음
 */
void mm_destroy(void);

/* ============ Debug Functions ============ */

/**
 * mm_dump_stats - 통계를 화면에 출력
 *
 * 디버그용 함수 (printf 사용하지 않음, syscall로 출력)
 */
void mm_dump_stats(void);

/**
 * mm_validate - 힙 무결성 검증
 *
 * Free-list 일관성 검사
 * 반환: 정상이면 0, 오류면 음수
 */
int mm_validate(void);

#endif /* MYOS_MM_H */
