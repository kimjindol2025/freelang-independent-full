#include "kvstore.h"
#include "mm.h"
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define KVSTORE_MAGIC 0x4B565354  /* KVST */
#define KVSTORE_VERSION 1

typedef struct {
    uint32_t key_len;
    uint32_t value_len;
    char *key;
    char *value;
} KVEntry;

static int write_u32(int fd, uint32_t value) {
    uint8_t buf[4] = {
        (uint8_t)(value >> 24),
        (uint8_t)(value >> 16),
        (uint8_t)(value >> 8),
        (uint8_t)value
    };
    return write(fd, buf, 4) == 4 ? 0 : -1;
}

static int read_u32(int fd, uint32_t *value) {
    uint8_t buf[4];
    if (read(fd, buf, 4) != 4) return -1;
    *value = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
             ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
    return 0;
}

KVStore* kvstore_open(const char *path) {
    if (!path) return NULL;

    KVStore *store = (KVStore *)mm_alloc(sizeof(KVStore));
    if (!store) return NULL;

    store->path = (char *)mm_alloc(strlen(path) + 1);
    if (!store->path) {
        mm_free(store);
        return NULL;
    }
    strcpy(store->path, path);

    /* 파일 열기 또는 생성 */
    int fd = open(path, O_RDWR, 0644);
    if (fd < 0) {
        /* 새 파일 생성 */
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            mm_free(store->path);
            mm_free(store);
            return NULL;
        }

        /* 헤더 작성 */
        write_u32(fd, KVSTORE_MAGIC);
        write_u32(fd, KVSTORE_VERSION);
        write_u32(fd, 0);  /* entry_count = 0 */

        close(fd);

        /* 다시 읽기 모드로 열기 */
        fd = open(path, O_RDWR, 0644);
        if (fd < 0) {
            mm_free(store->path);
            mm_free(store);
            return NULL;
        }
    }

    /* 헤더 검증 및 entry_count 읽기 */
    uint32_t magic, version;
    lseek(fd, 0, SEEK_SET);
    if (read_u32(fd, &magic) < 0 || magic != KVSTORE_MAGIC) {
        close(fd);
        mm_free(store->path);
        mm_free(store);
        return NULL;
    }
    if (read_u32(fd, &version) < 0 || version != KVSTORE_VERSION) {
        close(fd);
        mm_free(store->path);
        mm_free(store);
        return NULL;
    }
    if (read_u32(fd, &store->entry_count) < 0) {
        close(fd);
        mm_free(store->path);
        mm_free(store);
        return NULL;
    }

    store->fd = fd;
    return store;
}

void kvstore_close(KVStore *store) {
    if (!store) return;

    if (store->fd >= 0) {
        /* entry_count 업데이트 */
        lseek(store->fd, 8, SEEK_SET);
        write_u32(store->fd, store->entry_count);
        close(store->fd);
    }

    if (store->path) mm_free(store->path);
    mm_free(store);
}

int kvstore_put(KVStore *store, const char *key, const char *value) {
    if (!store || !key || !value) return -1;

    uint32_t key_len = strlen(key);
    uint32_t value_len = strlen(value);

    if (key_len > 1024 || value_len > 8192) return -1;

    /* 파일 끝으로 이동 */
    lseek(store->fd, 0, SEEK_END);

    /* 엔트리 작성 */
    if (write_u32(store->fd, key_len) < 0) return -1;
    if (write(store->fd, key, key_len) != (int)key_len) return -1;
    if (write_u32(store->fd, value_len) < 0) return -1;
    if (write(store->fd, value, value_len) != (int)value_len) return -1;

    store->entry_count++;
    return 0;
}

int kvstore_get(KVStore *store, const char *key, char *value, size_t value_size) {
    if (!store || !key || !value) return -1;

    uint32_t key_len = strlen(key);
    lseek(store->fd, 12, SEEK_SET);  /* 헤더 스킵 */

    for (uint32_t i = 0; i < store->entry_count; i++) {
        uint32_t k_len, v_len;
        char k_buf[1024];

        if (read_u32(store->fd, &k_len) < 0) return -1;
        if (k_len > sizeof(k_buf)) {
            lseek(store->fd, k_len + 4, SEEK_CUR);
            continue;
        }

        if (read(store->fd, k_buf, k_len) != (int)k_len) return -1;

        if (read_u32(store->fd, &v_len) < 0) return -1;

        if (k_len == key_len && strncmp(k_buf, key, key_len) == 0) {
            if (v_len >= value_size) return -1;
            if (read(store->fd, value, v_len) != (int)v_len) return -1;
            value[v_len] = '\0';
            return 0;
        }

        lseek(store->fd, v_len, SEEK_CUR);
    }

    return -1;  /* 찾지 못함 */
}

int kvstore_delete(KVStore *store, const char *key) {
    if (!store || !key) return -1;

    /* 간단한 구현: 새 파일에 제외하고 복사 */
    uint32_t key_len = strlen(key);
    char temp_path[256];
    strcpy(temp_path, store->path);
    strcat(temp_path, ".tmp");

    int tmp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (tmp_fd < 0) return -1;

    /* 헤더 복사 */
    write_u32(tmp_fd, KVSTORE_MAGIC);
    write_u32(tmp_fd, KVSTORE_VERSION);
    write_u32(tmp_fd, store->entry_count - 1);

    /* 엔트리 복사 (삭제할 것 제외) */
    lseek(store->fd, 12, SEEK_SET);
    for (uint32_t i = 0; i < store->entry_count; i++) {
        uint32_t k_len, v_len;
        char k_buf[1024];
        char v_buf[8192];

        if (read_u32(store->fd, &k_len) < 0) break;
        if (read(store->fd, k_buf, k_len) != (int)k_len) break;
        if (read_u32(store->fd, &v_len) < 0) break;
        if (read(store->fd, v_buf, v_len) != (int)v_len) break;

        if (k_len != key_len || strncmp(k_buf, key, key_len) != 0) {
            write_u32(tmp_fd, k_len);
            write(tmp_fd, k_buf, k_len);
            write_u32(tmp_fd, v_len);
            write(tmp_fd, v_buf, v_len);
        }
    }

    close(tmp_fd);
    close(store->fd);

    /* 파일 교체 */
    unlink(store->path);
    rename(temp_path, store->path);

    /* 다시 열기 */
    store->fd = open(store->path, O_RDWR, 0644);
    store->entry_count--;

    return 0;
}

int kvstore_iterate(KVStore *store,
                    int (*callback)(const char *key, const char *value, void *ctx),
                    void *ctx) {
    if (!store || !callback) return -1;

    lseek(store->fd, 12, SEEK_SET);

    for (uint32_t i = 0; i < store->entry_count; i++) {
        uint32_t k_len, v_len;
        char k_buf[1024];
        char v_buf[8192];

        if (read_u32(store->fd, &k_len) < 0) return -1;
        if (k_len > sizeof(k_buf)) return -1;
        if (read(store->fd, k_buf, k_len) != (int)k_len) return -1;
        k_buf[k_len] = '\0';

        if (read_u32(store->fd, &v_len) < 0) return -1;
        if (v_len > sizeof(v_buf)) return -1;
        if (read(store->fd, v_buf, v_len) != (int)v_len) return -1;
        v_buf[v_len] = '\0';

        if (callback(k_buf, v_buf, ctx) < 0) return -1;
    }

    return 0;
}

uint32_t kvstore_size(KVStore *store) {
    return store ? store->entry_count : 0;
}
