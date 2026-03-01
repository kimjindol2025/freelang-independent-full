#include "runtime.h"
#include "serializer.h"
#include "mm.h"
#include <string.h>

static void* my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static uint32_t default_hash(const void *k, size_t ks) {
    uint32_t h = 5381; const uint8_t *b = (const uint8_t *)k;
    for (size_t i = 0; i < ks; i++) h = ((h << 5) + h) + b[i];
    return h;
}

static int default_cmp(const void *k1, const void *k2) {
    return (*(int*)k1 == *(int*)k2) ? 0 : 1;
}

RuntimeContext* runtime_new(void) {
    RuntimeContext *ctx = (RuntimeContext *)mm_alloc(sizeof(RuntimeContext));
    if (!ctx) return NULL;
    
    ctx->vectors = vector_new(sizeof(Vector*));
    ctx->strings = (String **)mm_alloc(sizeof(String*) * 16);
    ctx->lists = (LinkedList **)mm_alloc(sizeof(LinkedList*) * 16);
    ctx->maps = (HashMap **)mm_alloc(sizeof(HashMap*) * 16);
    
    if (!ctx->vectors || !ctx->strings || !ctx->lists || !ctx->maps) {
        runtime_free(ctx);
        return NULL;
    }
    
    ctx->vector_count = 0;
    ctx->string_count = 0;
    ctx->list_count = 0;
    ctx->map_count = 0;
    
    ctx->string_capacity = 16;
    ctx->list_capacity = 16;
    ctx->map_capacity = 16;
    
    return ctx;
}

void runtime_free(RuntimeContext *ctx) {
    if (!ctx) return;
    
    if (ctx->vectors) {
        for (size_t i = 0; i < ctx->vector_count; i++) {
            Vector *v = *(Vector **)vector_at(ctx->vectors, i);
            if (v) vector_free(v);
        }
        vector_free(ctx->vectors);
    }
    
    if (ctx->strings) {
        for (size_t i = 0; i < ctx->string_count; i++) {
            if (ctx->strings[i]) string_free(ctx->strings[i]);
        }
        mm_free(ctx->strings);
    }
    
    if (ctx->lists) {
        for (size_t i = 0; i < ctx->list_count; i++) {
            if (ctx->lists[i]) list_free(ctx->lists[i]);
        }
        mm_free(ctx->lists);
    }
    
    if (ctx->maps) {
        for (size_t i = 0; i < ctx->map_count; i++) {
            if (ctx->maps[i]) hash_free(ctx->maps[i]);
        }
        mm_free(ctx->maps);
    }
    
    mm_free(ctx);
}

int runtime_load_vector(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                        size_t *out_id) {
    if (!ctx || !frame || !out_id) return -1;
    
    if (serializer_validate(frame, len) != 0) return -1;
    if (frame[SERIALIZER_HEADER_SIZE] != SERIALIZER_TYPE_VECTOR) return -1;
    
    uint32_t elem_size = serializer_read_u32(frame + SERIALIZER_HEADER_SIZE + 1);
    uint32_t count = serializer_read_u32(frame + SERIALIZER_HEADER_SIZE + 5);
    
    Vector *v = vector_new(elem_size);
    if (!v) return -1;
    
    const uint8_t *data_ptr = frame + SERIALIZER_HEADER_SIZE + 9;
    for (uint32_t i = 0; i < count; i++) {
        vector_push(v, data_ptr + (i * elem_size));
    }
    
    Vector *vptr = v;
    if (vector_push(ctx->vectors, &vptr) != 0) {
        vector_free(v);
        return -1;
    }
    
    *out_id = ctx->vector_count;
    ctx->vector_count++;
    return 0;
}

int runtime_load_string(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                        size_t *out_id) {
    if (!ctx || !frame || !out_id) return -1;
    
    if (serializer_validate(frame, len) != 0) return -1;
    if (frame[SERIALIZER_HEADER_SIZE] != SERIALIZER_TYPE_STRING) return -1;
    
    uint32_t str_len = serializer_read_u32(frame + SERIALIZER_HEADER_SIZE + 1);
    const char *str_data = (const char *)(frame + SERIALIZER_HEADER_SIZE + 5);
    
    String *s = string_new_with_capacity(str_len + 1);
    if (!s) return -1;
    
    if (str_len > 0) {
        char *buf = (char *)mm_alloc(str_len + 1);
        if (!buf) {
            string_free(s);
            return -1;
        }
        my_memcpy(buf, str_data, str_len);
        buf[str_len] = '\0';
        string_append(s, buf);
        mm_free(buf);
    }
    
    if (ctx->string_count >= ctx->string_capacity) {
        size_t new_cap = ctx->string_capacity * 2;
        String **new_strings = (String **)mm_alloc(sizeof(String*) * new_cap);
        if (!new_strings) {
            string_free(s);
            return -1;
        }
        my_memcpy(new_strings, ctx->strings, ctx->string_count * sizeof(String*));
        mm_free(ctx->strings);
        ctx->strings = new_strings;
        ctx->string_capacity = new_cap;
    }
    
    ctx->strings[ctx->string_count] = s;
    *out_id = ctx->string_count;
    ctx->string_count++;
    return 0;
}

int runtime_load_list(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                      size_t *out_id) {
    if (!ctx || !frame || !out_id) return -1;
    
    if (serializer_validate(frame, len) != 0) return -1;
    if (frame[SERIALIZER_HEADER_SIZE] != SERIALIZER_TYPE_LIST) return -1;
    
    uint32_t elem_size = serializer_read_u32(frame + SERIALIZER_HEADER_SIZE + 1);
    uint32_t count = serializer_read_u32(frame + SERIALIZER_HEADER_SIZE + 5);
    
    LinkedList *l = list_new(elem_size);
    if (!l) return -1;
    
    const uint8_t *data_ptr = frame + SERIALIZER_HEADER_SIZE + 9;
    for (uint32_t i = 0; i < count; i++) {
        list_push_back(l, data_ptr + (i * elem_size));
    }
    
    if (ctx->list_count >= ctx->list_capacity) {
        size_t new_cap = ctx->list_capacity * 2;
        LinkedList **new_lists = (LinkedList **)mm_alloc(sizeof(LinkedList*) * new_cap);
        if (!new_lists) {
            list_free(l);
            return -1;
        }
        my_memcpy(new_lists, ctx->lists, ctx->list_count * sizeof(LinkedList*));
        mm_free(ctx->lists);
        ctx->lists = new_lists;
        ctx->list_capacity = new_cap;
    }
    
    ctx->lists[ctx->list_count] = l;
    *out_id = ctx->list_count;
    ctx->list_count++;
    return 0;
}

int runtime_load_hashmap(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                         size_t *out_id) {
    if (!ctx || !frame || !out_id) return -1;
    
    if (serializer_validate(frame, len) != 0) return -1;
    if (frame[SERIALIZER_HEADER_SIZE] != SERIALIZER_TYPE_HASHMAP) return -1;
    
    HashMap *h = hash_new(sizeof(int), sizeof(int), default_hash, default_cmp);
    if (!h) return -1;
    
    if (ctx->map_count >= ctx->map_capacity) {
        size_t new_cap = ctx->map_capacity * 2;
        HashMap **new_maps = (HashMap **)mm_alloc(sizeof(HashMap*) * new_cap);
        if (!new_maps) {
            hash_free(h);
            return -1;
        }
        my_memcpy(new_maps, ctx->maps, ctx->map_count * sizeof(HashMap*));
        mm_free(ctx->maps);
        ctx->maps = new_maps;
        ctx->map_capacity = new_cap;
    }
    
    ctx->maps[ctx->map_count] = h;
    *out_id = ctx->map_count;
    ctx->map_count++;
    return 0;
}

Vector* runtime_get_vector(RuntimeContext *ctx, size_t id) {
    if (!ctx || id >= ctx->vector_count) return NULL;
    return *(Vector **)vector_at(ctx->vectors, id);
}

String* runtime_get_string(RuntimeContext *ctx, size_t id) {
    if (!ctx || id >= ctx->string_count) return NULL;
    return ctx->strings[id];
}

LinkedList* runtime_get_list(RuntimeContext *ctx, size_t id) {
    if (!ctx || id >= ctx->list_count) return NULL;
    return ctx->lists[id];
}

HashMap* runtime_get_hashmap(RuntimeContext *ctx, size_t id) {
    if (!ctx || id >= ctx->map_count) return NULL;
    return ctx->maps[id];
}

size_t runtime_vector_count(RuntimeContext *ctx) {
    return ctx ? ctx->vector_count : 0;
}

size_t runtime_string_count(RuntimeContext *ctx) {
    return ctx ? ctx->string_count : 0;
}

size_t runtime_list_count(RuntimeContext *ctx) {
    return ctx ? ctx->list_count : 0;
}

size_t runtime_map_count(RuntimeContext *ctx) {
    return ctx ? ctx->map_count : 0;
}
