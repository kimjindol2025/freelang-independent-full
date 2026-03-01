# MyOS HashMap Phase B-2 구현 완료 보고서

**날짜**: 2026-03-01
**상태**: ✅ **COMPLETE & VERIFIED**

---

## 📋 작업 요약

### 목표
> "자주독립 Phase B-2: 해시 테이블 (HashMap) 구현"

### 달성 내용

**✅ Open Addressing 기반 해시 테이블**

| 항목 | 상태 | 내용 |
|------|------|------|
| **코드 구현** | ✅ | hash.h (360줄) + hash.c (620줄) |
| **테스트** | ✅ | 47/47 테스트 통과 |
| **충돌 처리** | ✅ | Open Addressing with Linear Probing |
| **자동 리사이징** | ✅ | Load factor 기반 (0.75 임계값) |
| **Generic** | ✅ | int, string, struct 모든 타입 지원 |

---

## 📊 구현 통계

### 코드량
```
총 구현: 1,520줄
├─ hash.h:     360줄 (API + 문서)
├─ hash.c:     620줄 (Open Addressing 구현)
└─ test_hash.c: 540줄 (12개 테스트)
```

### 빌드 결과
```
libmyos_mm.a     26 KB  (mm.o + vector.o + hash.o)
test_hash        30 KB  (해시 테이블 테스트)
test_vector      30 KB  (벡터 테스트)
test_mm          21 KB  (메모리 매니저 테스트)
```

---

## 🧪 테스트 결과

### HashMap 테스트: 47/47 통과 ✅

```
╔════════════════════════════════════════╗
║  HashMap Test Summary                  ║
╚════════════════════════════════════════╝

Total Tests: 47
✅ Passed: 47
❌ Failed: 0

Success Rate: 100%

All 3 Test Suites Passed:
  - Memory Manager: 17/17 ✅
  - Vector: 46/46 ✅
  - HashMap: 47/47 ✅
  ─────────────────────
  Total: 110/110 ✅
```

### 테스트 커버리지

| 범주 | 테스트 | 상태 |
|------|--------|------|
| **생성/소멸** | new, free | ✅ |
| **저장/조회** | set, get | ✅ |
| **수정/삭제** | update, delete | ✅ |
| **검색** | contains, find | ✅ |
| **충돌 처리** | linear probing | ✅ |
| **리사이징** | 자동 확장 | ✅ |
| **다양한 키** | int, string | ✅ |
| **다양한 값** | int, struct | ✅ |
| **검증** | validate, dump | ✅ |

---

## 🎯 핵심 기능

### 1. Open Addressing (선형 탐사)

```c
/* 충돌 시 다음 슬롯 탐사 */

hash("key1") % 16 = 5
├─ slot[5] (empty) → 저장 성공

hash("key2") % 16 = 5 (충돌!)
├─ slot[5] (occupied)
├─ slot[6] (empty) → 저장 성공 (linear probing)

검색 시:
├─ hash("key2") % 16 = 5
├─ slot[5] 확인 (아님)
├─ slot[6] 확인 (맞음!) → 검색 성공
```

### 2. Load Factor 기반 리사이징

```c
Load Factor = count / capacity

리사이징 규칙:
  - load_factor > 0.75 → 용량 2배 확장
  - load_factor < 0.25 → 용량 축소 (선택사항)

목적:
  - 충돌 최소화
  - O(1) 평균 검색 시간 유지
  - 메모리 효율성

예시:
  capacity = 16, count = 12
  load_factor = 12/16 = 0.75
  → 리사이징: capacity = 32
```

### 3. 해시 함수

**제공되는 내장 함수**:
```c
uint32_t hash_djb2(key, size)       /* 범용 DJB2 */
uint32_t hash_int(key, size)        /* 정수 해시 */
uint32_t hash_string(key, size)     /* 문자열 해시 */
```

**DJB2 알고리즘**:
```
hash = 5381
for each byte:
    hash = ((hash << 5) + hash) + byte
    // hash * 33 + byte

장점:
  - 단순하고 빠름
  - 좋은 분포 특성
  - 범용적으로 사용 가능
```

### 4. 비교 함수

```c
int cmp_int(key1, key2)        /* 정수 비교 */
int cmp_string(key1, key2)     /* 문자열 비교 */
int cmp_bytes(key1, key2)      /* 바이트 배열 비교 */
```

**사용자 정의 함수 가능**:
```c
int cmp_custom(const void *a, const void *b) {
    // 반환: 0 (같음), < 0 (a < b), > 0 (a > b)
}

HashMap *map = hash_new(key_size, value_size,
                        hash_func, cmp_custom);
```

### 5. 15개 공개 API

**생성/소멸**:
- ✅ `hash_new()` - 테이블 생성
- ✅ `hash_free()` - 테이블 정리

**데이터 조작**:
- ✅ `hash_set()` - 저장 (새로 추가 또는 업데이트)
- ✅ `hash_get()` - 조회
- ✅ `hash_delete()` - 삭제
- ✅ `hash_contains()` - 키 존재 확인
- ✅ `hash_clear()` - 전체 제거

**쿼리/상태**:
- ✅ `hash_size()` - 엔트리 개수
- ✅ `hash_capacity()` - 슬롯 용량
- ✅ `hash_is_empty()` - 빈 테이블 확인
- ✅ `hash_load_factor()` - 로드 팩터 (%)

**최적화**:
- ✅ `hash_resize()` - 용량 명시적 설정

**순회**:
- ✅ `hash_foreach()` - 모든 엔트리 순회

**디버그**:
- ✅ `hash_dump()` - 통계 출력
- ✅ `hash_validate()` - 무결성 검증

---

## 📐 메모리 레이아웃

### HashMap 구조체

```c
typedef struct {
    void *entries;              /* HashEntry 배열 */
    size_t capacity;            /* 슬롯 개수 */
    size_t count;               /* 저장된 엔트리 */
    size_t key_size;            /* 키 크기 */
    size_t value_size;          /* 값 크기 */
    hash_func_t hash_func;      /* 해시 함수 포인터 */
    compare_func_t cmp_func;    /* 비교 함수 포인터 */
} HashMap;  /* 56 bytes (x86-64) */
```

### HashEntry 구조체

```c
typedef struct {
    void *key;                  /* 키 데이터 */
    void *value;                /* 값 데이터 */
    uint32_t hash;              /* 미리 계산한 해시 */
    int is_occupied;            /* 1: occupied, 0: empty, -1: deleted */
} HashEntry;  /* 32 bytes (x86-64) */
```

### 메모리 할당

```
HashMap 인스턴스 (mm_alloc)
├─ HashMap struct (56B)
│
└─ entries array (mm_alloc)
   ├─ HashEntry[0]: key, value, hash, state
   ├─ HashEntry[1]: ...
   └─ HashEntry[n]: ...

각 엔트리의 key/value도 mm_alloc으로 할당
```

---

## 🚀 성능 특성

### 시간 복잡도

| 연산 | 평균 | 최악 | 설명 |
|------|------|------|------|
| set | O(1) | O(n) | 리사이징 시 O(n) |
| get | O(1) | O(n) | 충돌 많을 때 O(n) |
| delete | O(1) | O(n) | 선형 탐사 |
| contains | O(1) | O(n) | 선형 탐사 |
| clear | O(n) | O(n) | 모든 엔트리 정리 |

### 공간 복잡도

```
HashMap 구조: 56 bytes (고정)
엔트리 배열: capacity * sizeof(HashEntry)
키 데이터: count * key_size (각각 mm_alloc)
값 데이터: count * value_size (각각 mm_alloc)

총 메모리 = 56 + capacity * 32 + count * (key_size + value_size)

할당율:
- 초기: 16 슬롯 (0 엔트리)
- 활성: 50~75% (자동 리사이징)
```

### 충돌 처리

```
Open Addressing + Linear Probing:

Best Case: O(1) - 첫 번째 슬롯에 발견
Average Case: O(1) - load factor < 0.75 유지
Worst Case: O(n) - 거의 모든 슬롯 탐사

선형 탐사의 장점:
  - 캐시 친화적 (sequential access)
  - 메모리 효율적 (추가 메모리 불필요)

단점:
  - 클러스터링 가능성
  - 최악의 경우 O(n)
  → load factor 제한으로 완화
```

---

## 🔍 Open Addressing 상세 분석

### 해시 테이블 상태 표현

```c
/* is_occupied 값의 의미 */
0:  빈 슬롯 (empty)
    - 검색: "키 없음" 반환
    - 삽입: 할당 가능

1:  사용 중 (occupied)
    - 검색: 키 비교
    - 삭제: is_occupied = -1로 변경

-1: 삭제됨 (tombstone)
    - 검색: 계속 탐사 (key chain 유지)
    - 삽입: 재할당 가능
```

### Lazy Deletion (게으른 삭제)

```c
삭제 과정:
1. 엔트리 찾기 (해시 → 선형 탐사)
2. is_occupied = -1 (tombstone 표시)
3. 키/값 메모리 해제
4. count-- (개수 감소)

장점:
  - 빠른 삭제 O(1)
  - key chain 유지 (검색 연속성)

단점:
  - 메모리 낭비 (tombstone 누적)
  → 리사이징 시 정리됨
```

### 리사이징 과정

```c
static int hash_grow(HashMap *map, size_t min_capacity) {
    1. 새 용량 계산 (2배)
    2. 새 엔트리 배열 할당
    3. 새 배열 초기화
    4. 기존 엔트리 재해시
       - 각 엔트리를 새 배열에 다시 삽입
       - tombstone 제거됨
    5. 기존 메모리 해제

시간: O(n) - 모든 엔트리 재해시
공간: O(n) - 새 배열 할당
```

---

## 📁 파일 구조

### freelang-independent/myos-lib/

```
myos-lib/
├── src/
│   ├── mm.h              (160줄, Phase A)
│   ├── mm.c              (560줄, Phase A)
│   ├── vector.h          (330줄, Phase B-1)
│   ├── vector.c          (540줄, Phase B-1)
│   ├── hash.h            (360줄, Phase B-2) ← NEW
│   └── hash.c            (620줄, Phase B-2) ← NEW
├── test_mm.c             (320줄, Phase A 테스트)
├── test_vector.c         (330줄, Phase B-1 테스트)
├── test_hash.c           (540줄, Phase B-2 테스트) ← NEW
├── Makefile              (모든 테스트 빌드)
├── IMPLEMENTATION_REPORT.md (Phase A)
├── VECTOR_IMPLEMENTATION.md (Phase B-1)
└── HASH_IMPLEMENTATION.md (이 파일)

라이브러리:
  libmyos_mm.a: 26 KB (모든 구현 통합)

테스트 바이너리:
  test_mm:     21 KB
  test_vector: 30 KB
  test_hash:   30 KB

커밋: 예정
```

---

## 💾 빌드 및 테스트 방법

### 빌드
```bash
cd /home/kimjin/freelang-independent/myos-lib
make clean
make
```

### 전체 테스트
```bash
make test
```

### 개별 테스트
```bash
make test-mm    # Memory Manager만
make test-vec   # Vector만
make test-hash  # HashMap만
```

### 라이브러리 사용
```bash
gcc -o myapp myapp.c \
  -L/path/to/myos-lib \
  -lmyos_mm \
  -I/path/to/myos-lib/src
```

---

## 📝 사용 예시

### 기본 사용 (정수 키)

```c
#include "src/hash.h"
#include "src/mm.h"

int main(void) {
    mm_init(1024 * 1024);

    /* 정수 → 정수 맵 */
    HashMap *map = hash_new(sizeof(int), sizeof(int),
                           hash_int, cmp_int);

    int key = 42;
    int value = 100;
    hash_set(map, &key, &value);

    int *found = (int*)hash_get(map, &key);
    printf("Value: %d\n", *found);  /* 100 */

    hash_free(map);
    mm_destroy();

    return 0;
}
```

### 문자열 키

```c
int main(void) {
    mm_init(1024 * 1024);

    /* 문자열 → 정수 맵 */
    HashMap *map = hash_new(sizeof(char*), sizeof(int),
                           hash_string, cmp_string);

    char *key = "hello";
    int value = 42;
    hash_set(map, &key, &value);

    int *found = (int*)hash_get(map, &key);
    printf("Value: %d\n", *found);  /* 42 */

    hash_free(map);
    mm_destroy();

    return 0;
}
```

### 구조체 값

```c
typedef struct {
    int id;
    char name[32];
} Person;

int main(void) {
    mm_init(1024 * 1024);

    /* 정수 → Person 구조체 맵 */
    HashMap *map = hash_new(sizeof(int), sizeof(Person),
                           hash_int, cmp_int);

    int id = 1;
    Person person = {1, "Alice"};
    hash_set(map, &id, &person);

    Person *found = (Person*)hash_get(map, &id);
    printf("%d: %s\n", found->id, found->name);

    hash_free(map);
    mm_destroy();

    return 0;
}
```

---

## 🎓 결론

**HashMap Phase B-2 상태**: 🟢 **COMPLETE & PRODUCTION-READY**

✅ **자주독립 Phase B-2 완성**
- Open Addressing 해시 테이블 구현
- 47/47 테스트 통과
- Linear probing으로 충돌 처리
- Load factor 기반 자동 리사이징
- Generic key-value 저장

**전체 진행률**: 50% (3/6 단계)

```
Phase A: Memory Manager   ✅ 100% (17/17)
Phase B-1: Vector        ✅ 100% (46/46)
Phase B-2: HashMap       ✅ 100% (47/47)
Phase B-3: String        ⏳ 0% (예정)
Phase C: Serializer      ⏳ 0% (예정)
Phase D: Runtime         ⏳ 0% (예정)

총 테스트: 110/110 ✅
코드량: 3,760줄
라이브러리: 26 KB
```

**다음 단계**: Phase B-3 - String Engine 구현

---

**생성**: 2026-03-01 11:30 KST
**상태**: ✅ **VERIFIED & READY TO COMMIT**
**저장소**: `/home/kimjin/freelang-independent/myos-lib/`
