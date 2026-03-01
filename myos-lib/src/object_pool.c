#include "object_pool.h"
#include "mm.h"
#include <string.h>

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

ObjectPool* pool_new(size_t capacity) {
    if (capacity == 0) return NULL;

    ObjectPool *pool = (ObjectPool *)mm_alloc(sizeof(ObjectPool));
    if (!pool) return NULL;

    pool->objects = (void **)mm_alloc(sizeof(void*) * capacity);
    pool->allocated = (uint8_t *)mm_alloc(sizeof(uint8_t) * capacity);

    if (!pool->objects || !pool->allocated) {
        if (pool->objects) mm_free(pool->objects);
        if (pool->allocated) mm_free(pool->allocated);
        mm_free(pool);
        return NULL;
    }

    my_memset(pool->allocated, 0, capacity);
    pool->capacity = capacity;
    pool->count = 0;

    return pool;
}

int pool_allocate(ObjectPool *pool, void *obj, size_t *out_id) {
    if (!pool || !obj || !out_id) return -1;

    /* First-fit: 처음으로 미할당된 슬롯 찾기 */
    for (size_t i = 0; i < pool->capacity; i++) {
        if (!pool->allocated[i]) {
            pool->objects[i] = obj;
            pool->allocated[i] = 1;
            pool->count++;
            *out_id = i;
            return 0;
        }
    }

    return -1;  /* 풀 가득 참 */
}

void* pool_get(ObjectPool *pool, size_t id) {
    if (!pool || id >= pool->capacity) return NULL;
    if (!pool->allocated[id]) return NULL;

    return pool->objects[id];
}

int pool_release(ObjectPool *pool, size_t id) {
    if (!pool || id >= pool->capacity) return -1;
    if (!pool->allocated[id]) return -1;

    pool->objects[id] = NULL;
    pool->allocated[id] = 0;
    pool->count--;

    return 0;
}

int pool_is_allocated(ObjectPool *pool, size_t id) {
    if (!pool || id >= pool->capacity) return -1;
    return pool->allocated[id] ? 1 : 0;
}

size_t pool_count(ObjectPool *pool) {
    return pool ? pool->count : 0;
}

void pool_free(ObjectPool *pool) {
    if (!pool) return;

    if (pool->objects) mm_free(pool->objects);
    if (pool->allocated) mm_free(pool->allocated);
    mm_free(pool);
}
