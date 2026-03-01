#ifndef MYOS_SNAPSHOT_H
#define MYOS_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Snapshot Management - 모든 로드된 객체를 파일로 저장/복원
 * ============================================================================ */

typedef struct {
    uint8_t **frames;           /* 직렬화된 모든 프레임 배열 */
    size_t *frame_sizes;        /* 각 프레임의 크기 */
    size_t frame_count;         /* 프레임 개수 */
    size_t frame_capacity;      /* 할당된 용량 */
    uint32_t snapshot_id;       /* 스냅샷 고유 ID */
    uint64_t timestamp;         /* 생성 시간 (unix timestamp) */
} Snapshot;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * 스냅샷 생성
 */
Snapshot* snapshot_create(void);

/**
 * 스냅샷에 프레임 추가
 * @param snap: 스냅샷 객체
 * @param frame: 직렬화된 바이너리 데이터
 * @param len: 프레임 길이
 * @return: 0 (성공), -1 (실패)
 */
int snapshot_add_frame(Snapshot *snap, const uint8_t *frame, size_t len);

/**
 * 스냅샷을 파일로 저장
 * Binary format:
 *   - Magic: 0x534E4150 (SNAP)
 *   - Version: 1 (uint32_t)
 *   - Snapshot ID: (uint32_t)
 *   - Timestamp: (uint64_t)
 *   - Frame count: (uint32_t)
 *   - [For each frame]:
 *     - Frame size: (uint32_t)
 *     - Frame data: (uint8_t[])
 *   - CRC32: (uint32_t)
 */
int snapshot_save(Snapshot *snap, const char *filename);

/**
 * 파일에서 스냅샷 로드
 */
Snapshot* snapshot_load(const char *filename);

/**
 * 스냅샷 해제
 */
void snapshot_free(Snapshot *snap);

/**
 * 스냅샷에서 프레임 개수 조회
 */
size_t snapshot_frame_count(Snapshot *snap);

/**
 * 스냅샷에서 특정 프레임 조회
 */
int snapshot_get_frame(Snapshot *snap, size_t index, uint8_t **out_frame, size_t *out_len);

#endif
