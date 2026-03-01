#include <stdio.h>
#include "src/mm.h"
#include "src/vector.h"
#include "src/string.h"
#include "src/list.h"
#include "src/hash.h"
#include "src/serializer.h"
#include "src/snapshot.h"

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

void test_snapshot_create(void) {
    TEST("D-2.1: Snapshot creation");

    Snapshot *snap = snapshot_create();
    ASSERT(snap != NULL, "Snapshot created");
    ASSERT(snapshot_frame_count(snap) == 0, "Initial count is 0");

    snapshot_free(snap);
}

void test_snapshot_add_frames(void) {
    TEST("D-2.2: Add frames to snapshot");

    Snapshot *snap = snapshot_create();
    ASSERT(snap != NULL, "Snapshot created");

    /* 여러 프레임 추가 */
    Vector *v1 = vector_new(sizeof(int));
    int val1 = 42;
    vector_push(v1, &val1);
    uint8_t *enc1 = NULL; size_t len1 = 0;
    serializer_encode_vector(v1, &enc1, &len1);

    int ret1 = snapshot_add_frame(snap, enc1, len1);
    ASSERT(ret1 == 0, "First frame added");

    String *s = string_new_with_capacity(16);
    string_append(s, "hello");
    uint8_t *enc2 = NULL; size_t len2 = 0;
    serializer_encode_string(s, &enc2, &len2);

    int ret2 = snapshot_add_frame(snap, enc2, len2);
    ASSERT(ret2 == 0, "Second frame added");

    ASSERT(snapshot_frame_count(snap) == 2, "Frame count is 2");

    mm_free(enc1);
    mm_free(enc2);
    vector_free(v1);
    string_free(s);
    snapshot_free(snap);
}

void test_snapshot_save_load(void) {
    TEST("D-2.3: Save and load snapshot");

    /* Create snapshot with frames */
    Snapshot *snap_orig = snapshot_create();
    ASSERT(snap_orig != NULL, "Original snapshot created");

    Vector *v = vector_new(sizeof(int));
    int vals[] = {10, 20, 30};
    for (int i = 0; i < 3; i++) vector_push(v, &vals[i]);
    uint8_t *enc = NULL; size_t enc_len = 0;
    serializer_encode_vector(v, &enc, &enc_len);

    int ret = snapshot_add_frame(snap_orig, enc, enc_len);
    ASSERT(ret == 0, "Frame added");

    /* Save */
    ret = snapshot_save(snap_orig, "/tmp/test_snapshot.bin");
    ASSERT(ret == 0, "Snapshot saved");

    /* Load */
    Snapshot *snap_loaded = snapshot_load("/tmp/test_snapshot.bin");
    ASSERT(snap_loaded != NULL, "Snapshot loaded");
    ASSERT(snapshot_frame_count(snap_loaded) == 1, "Frame count matches");

    mm_free(enc);
    vector_free(v);
    snapshot_free(snap_orig);
    snapshot_free(snap_loaded);
}

void test_snapshot_restore_data(void) {
    TEST("D-2.4: Restore data from snapshot");

    /* Create and save snapshot */
    Snapshot *snap = snapshot_create();

    Vector *v_orig = vector_new(sizeof(int));
    int vals[] = {100, 200, 300};
    for (int i = 0; i < 3; i++) vector_push(v_orig, &vals[i]);
    uint8_t *enc = NULL; size_t enc_len = 0;
    serializer_encode_vector(v_orig, &enc, &enc_len);

    snapshot_add_frame(snap, enc, enc_len);
    snapshot_save(snap, "/tmp/test_snapshot2.bin");

    /* Load snapshot and verify frame */
    Snapshot *snap_loaded = snapshot_load("/tmp/test_snapshot2.bin");
    ASSERT(snap_loaded != NULL, "Snapshot loaded");

    uint8_t *frame = NULL; size_t frame_len = 0;
    int ret = snapshot_get_frame(snap_loaded, 0, &frame, &frame_len);
    ASSERT(ret == 0, "Frame retrieved");
    ASSERT(frame_len == enc_len, "Frame size matches");

    /* Verify frame header */
    ASSERT(frame[11] == SERIALIZER_TYPE_VECTOR, "Frame type is VECTOR");

    mm_free(enc);
    vector_free(v_orig);
    snapshot_free(snap);
    snapshot_free(snap_loaded);
}

void test_snapshot_multiple_frames(void) {
    TEST("D-2.5: Multiple frame snapshot");

    Snapshot *snap = snapshot_create();

    /* Add Vector */
    Vector *v = vector_new(sizeof(int));
    int val1 = 42;
    vector_push(v, &val1);
    uint8_t *enc_v = NULL; size_t len_v = 0;
    serializer_encode_vector(v, &enc_v, &len_v);
    snapshot_add_frame(snap, enc_v, len_v);

    /* Add String */
    String *s = string_new_with_capacity(16);
    string_append(s, "test");
    uint8_t *enc_s = NULL; size_t len_s = 0;
    serializer_encode_string(s, &enc_s, &len_s);
    snapshot_add_frame(snap, enc_s, len_s);

    /* Add LinkedList */
    LinkedList *l = list_new(sizeof(int));
    int val2 = 99;
    list_push_back(l, &val2);
    uint8_t *enc_l = NULL; size_t len_l = 0;
    serializer_encode_list(l, &enc_l, &len_l);
    snapshot_add_frame(snap, enc_l, len_l);

    ASSERT(snapshot_frame_count(snap) == 3, "3 frames added");

    /* Save and load */
    snapshot_save(snap, "/tmp/test_snapshot3.bin");
    Snapshot *snap_loaded = snapshot_load("/tmp/test_snapshot3.bin");
    ASSERT(snapshot_frame_count(snap_loaded) == 3, "All frames restored");

    /* Verify frame types */
    uint8_t *frame1 = NULL; size_t len1 = 0;
    uint8_t *frame2 = NULL; size_t len2 = 0;
    uint8_t *frame3 = NULL; size_t len3 = 0;
    snapshot_get_frame(snap_loaded, 0, &frame1, &len1);
    snapshot_get_frame(snap_loaded, 1, &frame2, &len2);
    snapshot_get_frame(snap_loaded, 2, &frame3, &len3);

    ASSERT(frame1[11] == SERIALIZER_TYPE_VECTOR, "Frame 1 is VECTOR");
    ASSERT(frame2[11] == SERIALIZER_TYPE_STRING, "Frame 2 is STRING");
    ASSERT(frame3[11] == SERIALIZER_TYPE_LIST, "Frame 3 is LIST");

    mm_free(enc_v);
    mm_free(enc_s);
    mm_free(enc_l);
    vector_free(v);
    string_free(s);
    list_free(l);
    snapshot_free(snap);
    snapshot_free(snap_loaded);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase D-2: Snapshot Management - 5 Tests\n");
    printf("═════════════════════════════════════════════════════\n");

    mm_init(8 * 1024 * 1024);

    test_snapshot_create();
    test_snapshot_add_frames();
    test_snapshot_save_load();
    test_snapshot_restore_data();
    test_snapshot_multiple_frames();

    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");

    return (failed > 0) ? 1 : 0;
}
