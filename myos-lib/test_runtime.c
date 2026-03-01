#include <stdio.h>
#include "src/mm.h"
#include "src/vector.h"
#include "src/string.h"
#include "src/list.h"
#include "src/hash.h"
#include "src/serializer.h"
#include "src/runtime.h"

static int passed = 0, failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); passed++; } \
} while(0)

static uint32_t test_hash(const void *k, size_t ks) {
    uint32_t h = 5381; const uint8_t *b = (const uint8_t *)k;
    for (size_t i = 0; i < ks; i++) h = ((h << 5) + h) + b[i];
    return h;
}

static int test_cmp(const void *k1, const void *k2) {
    return (*(int*)k1 == *(int*)k2) ? 0 : 1;
}

void test_vector_load(void) {
    TEST("D-1.1: Vector decode");
    
    Vector *v_orig = vector_new(sizeof(int));
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) vector_push(v_orig, &vals[i]);
    
    uint8_t *enc = NULL; size_t enc_len = 0;
    serializer_encode_vector(v_orig, &enc, &enc_len);
    
    RuntimeContext *ctx = runtime_new();
    ASSERT(ctx != NULL, "Runtime context created");
    
    size_t vid = 0;
    int ret = runtime_load_vector(ctx, enc, enc_len, &vid);
    ASSERT(ret == 0, "Vector loaded");
    
    Vector *v_loaded = runtime_get_vector(ctx, vid);
    ASSERT(v_loaded != NULL, "Vector retrieved");
    ASSERT(vector_size(v_loaded) == 3, "Size matches");
    
    int *elem = (int *)vector_at(v_loaded, 0);
    ASSERT(elem && *elem == 10, "First element correct");
    
    mm_free(enc);
    vector_free(v_orig);
    runtime_free(ctx);
}

void test_string_load(void) {
    TEST("D-1.2: String decode");
    
    String *s_orig = string_new_with_capacity(16);
    string_append(s_orig, "hello");
    
    uint8_t *enc = NULL; size_t enc_len = 0;
    serializer_encode_string(s_orig, &enc, &enc_len);
    
    RuntimeContext *ctx = runtime_new();
    size_t sid = 0;
    int ret = runtime_load_string(ctx, enc, enc_len, &sid);
    ASSERT(ret == 0, "String loaded");
    
    String *s_loaded = runtime_get_string(ctx, sid);
    ASSERT(s_loaded != NULL, "String retrieved");
    
    mm_free(enc);
    string_free(s_orig);
    runtime_free(ctx);
}

void test_list_load(void) {
    TEST("D-1.3: LinkedList decode");
    
    LinkedList *l_orig = list_new(sizeof(int));
    int vals[] = {100, 200, 300};
    for (int i = 0; i < 3; i++) list_push_back(l_orig, &vals[i]);
    
    uint8_t *enc = NULL; size_t enc_len = 0;
    serializer_encode_list(l_orig, &enc, &enc_len);
    
    RuntimeContext *ctx = runtime_new();
    size_t lid = 0;
    int ret = runtime_load_list(ctx, enc, enc_len, &lid);
    ASSERT(ret == 0, "List loaded");
    
    LinkedList *l_loaded = runtime_get_list(ctx, lid);
    ASSERT(l_loaded != NULL, "List retrieved");
    ASSERT(list_size(l_loaded) == 3, "Size matches");
    
    int *elem = (int *)list_at(l_loaded, 0);
    ASSERT(elem && *elem == 100, "First element correct");
    
    mm_free(enc);
    list_free(l_orig);
    runtime_free(ctx);
}

void test_hashmap_load(void) {
    TEST("D-1.4: HashMap decode");
    
    HashMap *h_orig = hash_new(sizeof(int), sizeof(int), test_hash, test_cmp);
    ASSERT(h_orig != NULL, "HashMap created");
    
    uint8_t *enc = NULL; size_t enc_len = 0;
    serializer_encode_hashmap(h_orig, &enc, &enc_len);
    
    RuntimeContext *ctx = runtime_new();
    size_t mid = 0;
    int ret = runtime_load_hashmap(ctx, enc, enc_len, &mid);
    ASSERT(ret == 0, "HashMap loaded");
    
    HashMap *h_loaded = runtime_get_hashmap(ctx, mid);
    ASSERT(h_loaded != NULL, "HashMap retrieved");
    
    mm_free(enc);
    hash_free(h_orig);
    runtime_free(ctx);
}

void test_mixed_load(void) {
    TEST("D-1.5: Mixed object load");
    
    RuntimeContext *ctx = runtime_new();
    ASSERT(ctx != NULL, "Context created");
    
    Vector *v = vector_new(sizeof(int));
    int val = 42;
    vector_push(v, &val);
    uint8_t *enc_v = NULL; size_t len_v = 0;
    serializer_encode_vector(v, &enc_v, &len_v);
    
    size_t vid = 0;
    runtime_load_vector(ctx, enc_v, len_v, &vid);
    
    String *s = string_new_with_capacity(16);
    string_append(s, "test");
    uint8_t *enc_s = NULL; size_t len_s = 0;
    serializer_encode_string(s, &enc_s, &len_s);
    
    size_t sid = 0;
    runtime_load_string(ctx, enc_s, len_s, &sid);
    
    ASSERT(runtime_vector_count(ctx) == 1, "Vector count OK");
    ASSERT(runtime_string_count(ctx) == 1, "String count OK");
    ASSERT(runtime_get_vector(ctx, vid) != NULL, "Vector accessible");
    ASSERT(runtime_get_string(ctx, sid) != NULL, "String accessible");
    
    mm_free(enc_v);
    mm_free(enc_s);
    vector_free(v);
    string_free(s);
    runtime_free(ctx);
}

void test_error_handling(void) {
    TEST("D-1.6: Error handling");
    
    RuntimeContext *ctx = runtime_new();
    uint8_t bad_frame[20] = {0};
    
    size_t id = 0;
    int ret = runtime_load_vector(ctx, bad_frame, 20, &id);
    ASSERT(ret != 0, "Bad frame rejected");
    
    Vector *v = runtime_get_vector(ctx, 999);
    ASSERT(v == NULL, "Invalid ID returns NULL");
    
    runtime_free(ctx);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase D-1: Runtime Object Loader - 6 Tests\n");
    printf("═════════════════════════════════════════════════════\n");
    
    mm_init(8 * 1024 * 1024);
    
    test_vector_load();
    test_string_load();
    test_list_load();
    test_hashmap_load();
    test_mixed_load();
    test_error_handling();
    
    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");
    
    return (failed > 0) ? 1 : 0;
}
