#include <stdio.h>
#include "src/mm.h"
#include "src/vector.h"
#include "src/string.h"
#include "src/list.h"
#include "src/hash.h"
#include "src/serializer.h"
#include "src/crc32.h"

static int tests_passed = 0, tests_failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); tests_failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); tests_passed++; } \
} while(0)

static uint32_t default_hash(const void *k, size_t ks) {
    uint32_t h = 5381; const uint8_t *b = (const uint8_t *)k;
    for (size_t i = 0; i < ks; i++) h = ((h << 5) + h) + b[i];
    return h;
}

static int default_compare(const void *k1, const void *k2) {
    return (*(int*)k1 == *(int*)k2) ? 0 : 1;
}

void test_crc32(void) {
    TEST("CRC32: Basic");
    uint32_t crc = crc32_calculate((const uint8_t *)"hello", 5);
    ASSERT(crc != 0, "CRC32 not zero");
}

void test_vector(void) {
    TEST("Vector encoding");
    Vector *v = vector_new(sizeof(int));
    int vals[] = {1, 2, 3};
    for (int i = 0; i < 3; i++) vector_push(v, &vals[i]);
    uint8_t *enc = NULL; size_t len = 0;
    int r = serializer_encode_vector(v, &enc, &len);
    ASSERT(r == 0 && enc != NULL, "Encode OK");
    ASSERT(enc[11] == SERIALIZER_TYPE_VECTOR, "Type OK");
    ASSERT(serializer_validate(enc, len) == 0, "Validation OK");
    mm_free(enc); vector_free(v);
}

void test_string(void) {
    TEST("String encoding");
    String *s = string_new_with_capacity(16);
    string_append(s, "test");
    uint8_t *enc = NULL; size_t len = 0;
    int r = serializer_encode_string(s, &enc, &len);
    ASSERT(r == 0 && enc != NULL, "Encode OK");
    ASSERT(enc[11] == SERIALIZER_TYPE_STRING, "Type OK");
    ASSERT(serializer_validate(enc, len) == 0, "Validation OK");
    mm_free(enc); string_free(s);
}

void test_list(void) {
    TEST("LinkedList encoding");
    LinkedList *l = list_new(sizeof(int));
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) list_push_back(l, &vals[i]);
    uint8_t *enc = NULL; size_t len = 0;
    int r = serializer_encode_list(l, &enc, &len);
    ASSERT(r == 0 && enc != NULL, "Encode OK");
    ASSERT(enc[11] == SERIALIZER_TYPE_LIST, "Type OK");
    ASSERT(serializer_validate(enc, len) == 0, "Validation OK");
    mm_free(enc); list_free(l);
}

void test_bytes(void) {
    TEST("Bytes encoding");
    uint8_t data[] = {1, 2, 3, 4, 5};
    uint8_t *enc = NULL; size_t len = 0;
    int r = serializer_encode_bytes(data, 5, &enc, &len);
    ASSERT(r == 0 && enc != NULL, "Encode OK");
    ASSERT(enc[11] == SERIALIZER_TYPE_BYTES, "Type OK");
    ASSERT(serializer_validate(enc, len) == 0, "Validation OK");
    mm_free(enc);
}

void test_header(void) {
    TEST("Header parsing");
    Vector *v = vector_new(sizeof(int));
    int val = 42; vector_push(v, &val);
    uint8_t *enc = NULL; size_t len = 0;
    serializer_encode_vector(v, &enc, &len);
    uint8_t ver = 0; uint8_t tid = 0;
    int r = serializer_parse_header(enc, len, &ver, NULL, NULL, &tid);
    ASSERT(r == 0, "Parse OK");
    ASSERT(ver == SERIALIZER_VERSION, "Version OK");
    ASSERT(tid == SERIALIZER_TYPE_VECTOR, "Type OK");
    mm_free(enc); vector_free(v);
}

void test_hashmap(void) {
    TEST("HashMap encoding");
    HashMap *h = hash_new(sizeof(int), sizeof(int), default_hash, default_compare);
    uint8_t *enc = NULL; size_t len = 0;
    int r = serializer_encode_hashmap(h, &enc, &len);
    ASSERT(r == 0 && enc != NULL, "Encode OK");
    ASSERT(enc[11] == SERIALIZER_TYPE_HASHMAP, "Type OK");
    mm_free(enc); hash_free(h);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase C: Serializer Protocol v1.0 - 8 Tests\n");
    printf("═════════════════════════════════════════════════════\n");
    mm_init(8 * 1024 * 1024);
    test_crc32(); test_vector(); test_string(); test_list();
    test_bytes(); test_header(); test_hashmap();
    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", tests_passed, tests_failed);
    printf("═════════════════════════════════════════════════════\n\n");
    return (tests_failed > 0) ? 1 : 0;
}
