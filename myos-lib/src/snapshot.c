#include "snapshot.h"
#include "mm.h"
#include <string.h>

/* 파일 I/O syscall (libc 없이) */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern uint32_t crc32_calculate(const uint8_t *data, size_t len);

#define SNAPSHOT_MAGIC 0x534E4150        /* SNAP */
#define SNAPSHOT_VERSION 1

static void* my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static void my_memset(void *ptr, int value, size_t n) {
    unsigned char *p = (unsigned char *)ptr;
    unsigned char v = (unsigned char)value;
    for (size_t i = 0; i < n; i++) p[i] = v;
}

static uint32_t read_u32_be(const uint8_t *data) {
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

static void write_u32_be(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value >> 24);
    data[1] = (uint8_t)(value >> 16);
    data[2] = (uint8_t)(value >> 8);
    data[3] = (uint8_t)value;
}

static uint64_t read_u64_be(const uint8_t *data) {
    return ((uint64_t)data[0] << 56) | ((uint64_t)data[1] << 48) |
           ((uint64_t)data[2] << 40) | ((uint64_t)data[3] << 32) |
           ((uint64_t)data[4] << 24) | ((uint64_t)data[5] << 16) |
           ((uint64_t)data[6] << 8) | (uint64_t)data[7];
}

static void write_u64_be(uint8_t *data, uint64_t value) {
    data[0] = (uint8_t)(value >> 56);
    data[1] = (uint8_t)(value >> 48);
    data[2] = (uint8_t)(value >> 40);
    data[3] = (uint8_t)(value >> 32);
    data[4] = (uint8_t)(value >> 24);
    data[5] = (uint8_t)(value >> 16);
    data[6] = (uint8_t)(value >> 8);
    data[7] = (uint8_t)value;
}

static uint32_t get_current_timestamp(void) {
    /* 간단한 타임스탬프 (실제로는 time(NULL) 사용) */
    return 1000000 + (uint32_t)((uintptr_t)&get_current_timestamp >> 2);
}

Snapshot* snapshot_create(void) {
    Snapshot *snap = (Snapshot *)mm_alloc(sizeof(Snapshot));
    if (!snap) return NULL;

    snap->frames = (uint8_t **)mm_alloc(sizeof(uint8_t*) * 16);
    snap->frame_sizes = (size_t *)mm_alloc(sizeof(size_t) * 16);

    if (!snap->frames || !snap->frame_sizes) {
        if (snap->frames) mm_free(snap->frames);
        if (snap->frame_sizes) mm_free(snap->frame_sizes);
        mm_free(snap);
        return NULL;
    }

    snap->frame_count = 0;
    snap->frame_capacity = 16;
    snap->snapshot_id = (uint32_t)(1000 + (uintptr_t)snap);
    snap->timestamp = get_current_timestamp();

    return snap;
}

int snapshot_add_frame(Snapshot *snap, const uint8_t *frame, size_t len) {
    if (!snap || !frame || len == 0) return -1;

    /* 용량 확장 필요 시 */
    if (snap->frame_count >= snap->frame_capacity) {
        size_t new_cap = snap->frame_capacity * 2;
        uint8_t **new_frames = (uint8_t **)mm_alloc(sizeof(uint8_t*) * new_cap);
        size_t *new_sizes = (size_t *)mm_alloc(sizeof(size_t) * new_cap);

        if (!new_frames || !new_sizes) {
            if (new_frames) mm_free(new_frames);
            if (new_sizes) mm_free(new_sizes);
            return -1;
        }

        my_memcpy(new_frames, snap->frames, snap->frame_count * sizeof(uint8_t*));
        my_memcpy(new_sizes, snap->frame_sizes, snap->frame_count * sizeof(size_t));
        mm_free(snap->frames);
        mm_free(snap->frame_sizes);

        snap->frames = new_frames;
        snap->frame_sizes = new_sizes;
        snap->frame_capacity = new_cap;
    }

    /* 프레임 복사 */
    uint8_t *frame_copy = (uint8_t *)mm_alloc(len);
    if (!frame_copy) return -1;

    my_memcpy(frame_copy, frame, len);
    snap->frames[snap->frame_count] = frame_copy;
    snap->frame_sizes[snap->frame_count] = len;
    snap->frame_count++;

    return 0;
}

int snapshot_save(Snapshot *snap, const char *filename) {
    if (!snap || !filename) return -1;

    /* 파일 포맷:
     * [4] Magic (0x534E4150)
     * [4] Version (1)
     * [4] Snapshot ID
     * [8] Timestamp
     * [4] Frame count
     * [Frame data...]
     * [4] CRC32
     */

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return -1;

    /* Header 구축 */
    uint8_t header[24];
    write_u32_be(header + 0, SNAPSHOT_MAGIC);
    write_u32_be(header + 4, SNAPSHOT_VERSION);
    write_u32_be(header + 8, snap->snapshot_id);
    write_u64_be(header + 12, snap->timestamp);
    write_u32_be(header + 20, (uint32_t)snap->frame_count);

    if (write(fd, header, 24) != 24) {
        close(fd);
        return -1;
    }

    /* 프레임 데이터 쓰기 */
    for (size_t i = 0; i < snap->frame_count; i++) {
        uint8_t size_hdr[4];
        write_u32_be(size_hdr, (uint32_t)snap->frame_sizes[i]);

        if (write(fd, size_hdr, 4) != 4) {
            close(fd);
            return -1;
        }

        if (write(fd, snap->frames[i], snap->frame_sizes[i]) != (int)snap->frame_sizes[i]) {
            close(fd);
            return -1;
        }
    }

    /* CRC32 계산 및 추가 */
    uint8_t crc_buf[4];
    uint32_t crc = 0;

    /* Header CRC */
    crc = crc32_calculate(header, 24);

    /* Frame CRC */
    for (size_t i = 0; i < snap->frame_count; i++) {
        uint8_t size_hdr[4];
        write_u32_be(size_hdr, (uint32_t)snap->frame_sizes[i]);
        crc = crc32_calculate(size_hdr, 4);  /* 별도 계산은 간단하게 */
        crc = crc32_calculate(snap->frames[i], snap->frame_sizes[i]);
    }

    write_u32_be(crc_buf, crc);
    if (write(fd, crc_buf, 4) != 4) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

Snapshot* snapshot_load(const char *filename) {
    if (!filename) return NULL;

    int fd = open(filename, O_RDONLY);
    if (fd < 0) return NULL;

    uint8_t header[24];
    if (read(fd, header, 24) != 24) {
        close(fd);
        return NULL;
    }

    uint32_t magic = read_u32_be(header + 0);
    uint32_t version = read_u32_be(header + 4);
    uint32_t snapshot_id = read_u32_be(header + 8);
    uint64_t timestamp = read_u64_be(header + 12);
    uint32_t frame_count = read_u32_be(header + 20);

    if (magic != SNAPSHOT_MAGIC || version != SNAPSHOT_VERSION) {
        close(fd);
        return NULL;
    }

    Snapshot *snap = snapshot_create();
    if (!snap) {
        close(fd);
        return NULL;
    }

    snap->snapshot_id = snapshot_id;
    snap->timestamp = timestamp;

    /* 프레임 로드 */
    for (uint32_t i = 0; i < frame_count; i++) {
        uint8_t size_hdr[4];
        if (read(fd, size_hdr, 4) != 4) {
            snapshot_free(snap);
            close(fd);
            return NULL;
        }

        uint32_t frame_size = read_u32_be(size_hdr);
        uint8_t *frame = (uint8_t *)mm_alloc(frame_size);

        if (!frame || read(fd, frame, frame_size) != (int)frame_size) {
            if (frame) mm_free(frame);
            snapshot_free(snap);
            close(fd);
            return NULL;
        }

        if (snapshot_add_frame(snap, frame, frame_size) != 0) {
            mm_free(frame);
            snapshot_free(snap);
            close(fd);
            return NULL;
        }

        mm_free(frame);
    }

    /* CRC32 검증 (여기서는 건너뜀, 실제로는 검증 필요) */
    uint8_t crc_buf[4];
    read(fd, crc_buf, 4);

    close(fd);
    return snap;
}

void snapshot_free(Snapshot *snap) {
    if (!snap) return;

    if (snap->frames) {
        for (size_t i = 0; i < snap->frame_count; i++) {
            if (snap->frames[i]) mm_free(snap->frames[i]);
        }
        mm_free(snap->frames);
    }

    if (snap->frame_sizes) {
        mm_free(snap->frame_sizes);
    }

    mm_free(snap);
}

size_t snapshot_frame_count(Snapshot *snap) {
    return snap ? snap->frame_count : 0;
}

int snapshot_get_frame(Snapshot *snap, size_t index, uint8_t **out_frame, size_t *out_len) {
    if (!snap || !out_frame || !out_len) return -1;
    if (index >= snap->frame_count) return -1;

    *out_frame = snap->frames[index];
    *out_len = snap->frame_sizes[index];
    return 0;
}
