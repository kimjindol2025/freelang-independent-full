# Phase D: Runtime VM Binding - 설계

**날짜**: 2026-03-01
**상태**: 시작
**목표**: 직렬화된 데이터를 VM에서 로드/언로드하고 스냅샷으로 저장

---

## 🎯 목표

MyOS_Lib의 모든 자료구조를 다음과 같이 통합:

```
직렬화된 바이너리 → [Deserializer] → Runtime Object
Runtime Object → [Serializer] → 바이너리 저장 (스냅샷)
```

---

## 📋 Phase D 세부 계획

### D-1: Runtime Object Loader (Decoder 구현)
```c
/* src/runtime.h 신규 */
typedef struct {
    Vector *vectors;        // 로드된 Vector들
    String *strings;        // 로드된 String들
    LinkedList *lists;      // 로드된 LinkedList들
    HashMap *maps;          // 로드된 HashMap들
    size_t object_count;
} RuntimeContext;

RuntimeContext* runtime_new(void);
int runtime_load_vector(RuntimeContext *ctx, const uint8_t *frame, size_t len);
int runtime_load_string(RuntimeContext *ctx, const uint8_t *frame, size_t len);
int runtime_load_list(RuntimeContext *ctx, const uint8_t *frame, size_t len);
void runtime_free(RuntimeContext *ctx);
```

### D-2: Snapshot Management
```c
/* src/snapshot.h 신규 */
typedef struct {
    uint8_t **frames;       // 모든 Frame들
    size_t *frame_sizes;
    size_t frame_count;
    uint32_t snapshot_id;
    uint64_t timestamp;
} Snapshot;

Snapshot* snapshot_create(void);
int snapshot_add_frame(Snapshot *snap, const uint8_t *frame, size_t len);
int snapshot_save(Snapshot *snap, const char *filename);
Snapshot* snapshot_load(const char *filename);
void snapshot_free(Snapshot *snap);
```

### D-3: Object Pooling
```c
/* src/object_pool.h 신규 */
typedef struct {
    void **objects;         // 풀에 있는 객체들
    size_t *object_ids;
    size_t capacity;
    size_t count;
} ObjectPool;

ObjectPool* pool_new(size_t capacity);
int pool_allocate(ObjectPool *pool, void *obj, size_t *out_id);
void* pool_get(ObjectPool *pool, size_t id);
void pool_release(ObjectPool *pool, size_t id);
void pool_free(ObjectPool *pool);
```

---

## 📊 Test 계획 (16 tests)

### D-1: Decoder Tests (6 tests)
1. Vector 디코드
2. String 디코드
3. LinkedList 디코드
4. HashMap 디코드
5. 혼합 객체 로드
6. 에러 처리

### D-2: Snapshot Tests (5 tests)
1. 스냅샷 생성
2. Frame 추가
3. 스냅샷 저장
4. 스냅샷 로드
5. 복원 검증

### D-3: Object Pool Tests (5 tests)
1. 객체 할당
2. 객체 조회
3. 객체 해제
4. ID 재사용
5. 풀 용량 관리

---

## 🔄 구현 순서

| Step | 파일 | 역할 | 테스트 |
|------|------|------|--------|
| 1 | runtime.c/h | Decoder 구현 | test_runtime.c (6) |
| 2 | snapshot.c/h | 스냅샷 관리 | test_snapshot.c (5) |
| 3 | object_pool.c/h | 객체 풀 | test_pool.c (5) |

**총 16개 테스트**

---

## 💾 파일 구조

```
myos-lib/
├── src/
│   ├── runtime.c/h        (신규)
│   ├── snapshot.c/h       (신규)
│   ├── object_pool.c/h    (신규)
│   ├── serializer.c/h     (기존)
│   └── ...
├── test_runtime.c         (신규)
├── test_snapshot.c        (신규)
├── test_pool.c            (신규)
└── Makefile               (수정)
```

---

## 🚀 완성 기준

- ✅ 16개 테스트 모두 통과
- ✅ 500KB 바이너리 이하
- ✅ 메모리 누수 없음 (Valgrind)
- ✅ CRC32 검증으로 무결성 확보

---

## 📈 누적 진행도

```
Phase A: Memory (67) ✅
Phase B-1: Vector (56) ✅
Phase B-2: HashMap (48) ✅
Phase B-3: String (61) ✅
Phase B-4: LinkedList (74) ✅
Phase C: Serializer (18) ✅
Phase D: Runtime VM Binding (16) 🔨
━━━━━━━━━━━━━━━━━━━━━━━━━━━
총: 340 tests (100%)
6,000+ 줄 Zero-Dependency C
```

