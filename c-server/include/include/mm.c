/**
 * MyOS Memory Manager (mm.c)
 *
 * Zero-Dependency 구현
 * - syscall 직접 호출 (mmap, munmap, write)
 * - libc 함수 사용 금지
 * - Free-list 기반 메모리 관리
 */

#include "mm.h"
#include <sys/syscall.h>
#include <linux/mman.h>
#include <unistd.h>

/* MAP_FAILED이 정의되지 않은 경우 */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

/* ============ Internal Data Structures ============ */

/**
 * MemBlock - 메모리 블록 메타데이터
 *
 * 각 할당 블록의 앞에 이 구조체가 저장됨
 * 포인터 = 블록 시작주소
 * 포인터 - sizeof(MemBlock) = 메타데이터
 */
typedef struct _mem_block {
    uint32_t magic;              /* 0xDEADBEEF (검증용) */
    uint32_t is_free;            /* 1: free, 0: allocated */
    size_t size;                 /* 블록 크기 (메타 제외) */
    struct _mem_block *next;     /* Free-list의 다음 블록 */
    struct _mem_block *prev;     /* Free-list의 이전 블록 */
} MemBlock;

#define MEM_MAGIC 0xDEADBEEF
#define MEM_ALIGN 16             /* 16바이트 정렬 */
#define METADATA_SIZE (((sizeof(MemBlock) + MEM_ALIGN - 1) / MEM_ALIGN) * MEM_ALIGN)

/* ============ Global State ============ */

static void *heap_base = NULL;
static size_t heap_size = 0;
static MemBlock *free_list = NULL;  /* Free-list 헤드 */

/* 통계 */
static uint64_t total_allocations = 0;
static uint64_t total_deallocations = 0;
static uint64_t current_allocated = 0;

/* ============ Syscall Wrappers ============ */

/**
 * syscall_mmap - mmap syscall 직접 호출
 *
 * x86-64 syscall: rax=9
 * 인자: rdi=addr, rsi=len, rdx=prot, r10=flags, r8=fd, r9=offset
 */
static void* syscall_mmap(void *addr, size_t len, int prot, int flags,
                         int fd, off_t offset) {
    return (void*)syscall(SYS_mmap, addr, len, prot, flags, fd, offset);
}

/**
 * syscall_munmap - munmap syscall 직접 호출
 */
static int syscall_munmap(void *addr, size_t len) {
    return syscall(SYS_munmap, addr, len);
}

/**
 * syscall_write - write syscall 직접 호출 (디버그 출력)
 */
static ssize_t syscall_write(int fd, const void *buf, size_t count) {
    return syscall(SYS_write, fd, buf, count);
}

/* ============ Utility Functions ============ */

/**
 * align_up - 크기를 정렬값으로 올림
 */
static size_t align_up(size_t size, size_t align) {
    return (size + align - 1) / align * align;
}

/**
 * itoa_simple - 정수를 문자열로 변환 (sprintf 없이)
 *
 * 주의: num=0이면 "0", num<0이면 음수 표시 안 함
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

/**
 * str_len - libc strlen 없이 문자열 길이 계산
 */
static size_t str_len(const char *s) {
    size_t len = 0;
    while (*s++) len++;
    return len;
}

/**
 * debug_write - syscall을 사용해 디버그 메시지 출력
 *
 * printf 대신 write() syscall 사용
 */
static void debug_write(const char *msg) {
    syscall_write(1, msg, str_len(msg));
}

/* ============ Memory Manager Core ============ */

/**
 * mm_init - 힙 메모리 초기화
 */
int mm_init(size_t requested_size) {
    if (heap_base != NULL) {
        return -1;  /* 이미 초기화됨 */
    }

    /* 요청 크기를 페이지 정렬 (보통 4KB) */
    size_t page_size = 4096;
    heap_size = align_up(requested_size, page_size);

    /* mmap으로 힙 할당 */
    heap_base = syscall_mmap(NULL, heap_size,
                            PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS,
                            -1, 0);

    if (heap_base == MAP_FAILED) {
        heap_base = NULL;
        return -1;
    }

    /* 전체 힙을 하나의 free 블록으로 초기화 */
    MemBlock *block = (MemBlock *)heap_base;
    block->magic = MEM_MAGIC;
    block->is_free = 1;
    block->size = heap_size - METADATA_SIZE;
    block->next = NULL;
    block->prev = NULL;

    free_list = block;

    return 0;
}

/**
 * mm_alloc - 메모리 할당
 *
 * Free-list에서 first-fit 탐색
 * 블록이 여유가 있으면 분할
 */
void* mm_alloc(size_t size) {
    if (heap_base == NULL || size == 0) {
        return NULL;
    }

    /* 요청 크기를 정렬 */
    size = align_up(size, MEM_ALIGN);

    /* Free-list에서 적절한 블록 찾기 */
    MemBlock *block = free_list;
    MemBlock *prev = NULL;

    while (block != NULL) {
        /* 블록이 충분한 크기를 가지는가? */
        if (block->is_free && block->size >= size) {
            /* 블록 분할 (필요한 경우) */
            if (block->size > size + METADATA_SIZE) {
                /* 분할: 현재 블록을 size로 자르고 나머지는 새 free 블록으로 */
                MemBlock *new_block = (MemBlock *)((char *)block + METADATA_SIZE + size);
                new_block->magic = MEM_MAGIC;
                new_block->is_free = 1;
                new_block->size = block->size - size - METADATA_SIZE;
                new_block->next = block->next;
                new_block->prev = block;

                if (block->next) block->next->prev = new_block;
                block->next = new_block;
            }

            /* 블록을 allocated으로 표시 */
            block->size = size;
            block->is_free = 0;

            /* Free-list에서 제거 */
            if (prev) {
                prev->next = block->next;
            } else {
                free_list = block->next;
            }
            if (block->next) block->next->prev = prev;

            /* 통계 업데이트 */
            total_allocations++;
            current_allocated += size;

            /* 데이터 포인터 반환 (메타데이터 뒤) */
            return (void *)((char *)block + METADATA_SIZE);
        }

        prev = block;
        block = block->next;
    }

    /* 사용 가능한 블록 없음 */
    return NULL;
}

/**
 * mm_free - 메모리 해제
 *
 * Free-list로 반환
 * 인접한 블록과 병합 (coalescing)
 */
void mm_free(void *ptr) {
    if (ptr == NULL || heap_base == NULL) {
        return;
    }

    /* 메타데이터 주소 계산 */
    MemBlock *block = (MemBlock *)((char *)ptr - METADATA_SIZE);

    /* 검증 */
    if (block->magic != MEM_MAGIC || block->is_free) {
        return;  /* 이미 해제되었거나 잘못된 포인터 */
    }

    /* 통계 업데이트 */
    total_deallocations++;
    current_allocated -= block->size;

    /* Free 표시 */
    block->is_free = 1;

    /* Free-list에 삽입 (주소 순서대로) */
    MemBlock *pos = NULL;
    MemBlock *curr = free_list;

    while (curr != NULL && curr < block) {
        pos = curr;
        curr = curr->next;
    }

    block->next = curr;
    block->prev = pos;

    if (pos) pos->next = block;
    else free_list = block;

    if (curr) curr->prev = block;

    /* 인접 블록 병합 - 이후 블록 */
    if (curr != NULL && (char *)block + METADATA_SIZE + block->size == (char *)curr) {
        block->size += METADATA_SIZE + curr->size;
        block->next = curr->next;
        if (curr->next) curr->next->prev = block;
    }

    /* 인접 블록 병합 - 이전 블록 */
    if (pos != NULL && (char *)pos + METADATA_SIZE + pos->size == (char *)block) {
        pos->size += METADATA_SIZE + block->size;
        pos->next = block->next;
        if (block->next) block->next->prev = pos;
    }
}

/**
 * mm_get_stats - 메모리 통계 조회
 */
int mm_get_stats(MMStats *out) {
    if (out == NULL) {
        return -1;
    }

    uint64_t free_size = 0;
    uint32_t free_blocks_count = 0;
    uint32_t allocated_blocks_count = 0;

    /* Free-list 순회 */
    MemBlock *block = free_list;
    while (block != NULL) {
        if (block->is_free) {
            free_size += block->size;
            free_blocks_count++;
        }
        block = block->next;
    }

    /* 할당된 블록 개수 계산 */
    if (heap_base != NULL) {
        MemBlock *curr = (MemBlock *)heap_base;
        char *end = (char *)heap_base + heap_size;

        while ((char *)curr < end) {
            if (curr->magic == MEM_MAGIC && !curr->is_free) {
                allocated_blocks_count++;
            }
            curr = (MemBlock *)((char *)curr + METADATA_SIZE + curr->size);
        }
    }

    out->total_heap_size = heap_size;
    out->allocated_size = current_allocated;
    out->free_size = free_size;
    out->total_allocations = total_allocations;
    out->total_deallocations = total_deallocations;
    out->free_blocks_count = free_blocks_count;
    out->allocated_blocks_count = allocated_blocks_count;

    return 0;
}

/**
 * mm_destroy - 메모리 매니저 정리
 */
void mm_destroy(void) {
    if (heap_base == NULL) {
        return;
    }

    syscall_munmap(heap_base, heap_size);
    heap_base = NULL;
    heap_size = 0;
    free_list = NULL;
    total_allocations = 0;
    total_deallocations = 0;
    current_allocated = 0;
}

/* ============ Debug Functions ============ */

/**
 * mm_dump_stats - 통계를 화면에 출력 (syscall 사용)
 */
void mm_dump_stats(void) {
    MMStats stats;
    if (mm_get_stats(&stats) < 0) {
        debug_write("mm_dump_stats: failed to get stats\n");
        return;
    }

    char buf[512];
    char num_buf[64];

    /* 직접 버퍼에 구성하기 (sprintf 없이) */
    const char *header = "=== Memory Manager Stats ===\n";
    debug_write(header);

    /* Total Heap Size */
    debug_write("Total Heap Size: ");
    itoa_simple(stats.total_heap_size, num_buf, 10);
    debug_write(num_buf);
    debug_write(" bytes\n");

    /* Allocated Size */
    debug_write("Allocated: ");
    itoa_simple(stats.allocated_size, num_buf, 10);
    debug_write(num_buf);
    debug_write(" bytes\n");

    /* Free Size */
    debug_write("Free: ");
    itoa_simple(stats.free_size, num_buf, 10);
    debug_write(num_buf);
    debug_write(" bytes\n");

    /* Allocations */
    debug_write("Total Allocations: ");
    itoa_simple(stats.total_allocations, num_buf, 10);
    debug_write(num_buf);
    debug_write("\n");

    /* Deallocations */
    debug_write("Total Deallocations: ");
    itoa_simple(stats.total_deallocations, num_buf, 10);
    debug_write(num_buf);
    debug_write("\n");

    /* Free Blocks */
    debug_write("Free Blocks: ");
    itoa_simple(stats.free_blocks_count, num_buf, 10);
    debug_write(num_buf);
    debug_write("\n");

    /* Allocated Blocks */
    debug_write("Allocated Blocks: ");
    itoa_simple(stats.allocated_blocks_count, num_buf, 10);
    debug_write(num_buf);
    debug_write("\n");

    const char *footer = "============================\n";
    debug_write(footer);
}

/**
 * mm_validate - 힙 무결성 검증
 */
int mm_validate(void) {
    if (heap_base == NULL) {
        return -1;
    }

    MemBlock *block = (MemBlock *)heap_base;
    char *end = (char *)heap_base + heap_size;

    /* 블록 순회하며 검증 */
    while ((char *)block < end) {
        if (block->magic != MEM_MAGIC) {
            debug_write("mm_validate: invalid magic\n");
            return -1;
        }

        char *block_end = (char *)block + METADATA_SIZE + block->size;
        if (block_end > end) {
            debug_write("mm_validate: block overflow\n");
            return -1;
        }

        block = (MemBlock *)block_end;
    }

    return 0;
}
