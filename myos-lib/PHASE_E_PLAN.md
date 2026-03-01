# Phase E: SQL Engine - 설계

**날짜**: 2026-03-01  
**상태**: 계획 중  
**목표**: 경량 SQL 쿼리 엔진 구현

---

## 🎯 목표

MyOS_Lib 기반으로 간단한 SQL 쿼리 처리:
- CREATE TABLE, INSERT, SELECT, UPDATE, DELETE
- WHERE 조건 필터링
- 인덱스 기반 검색
- 메타데이터 관리

---

## 📋 Phase E 세부 계획

### E-1: Table Management (테이블 관리)
```c
typedef struct {
    char table_name[64];
    Vector *columns;         // Column 정의
    Vector *rows;            // 실제 데이터
    HashMap *indexes;        // 인덱스들
    size_t row_count;
} Table;

typedef struct {
    char name[32];
    uint8_t type;            // INT=1, TEXT=2, REAL=3
    size_t offset;           // 행 내에서의 위치
} Column;
```

**파일**:
```
src/table.h/c         (신규, ~300줄)
test_table.c          (신규, ~200줄)
```

**테스트** (6 tests):
1. 테이블 생성
2. 열 추가
3. 행 삽입
4. 행 삭제
5. 테이블 정보 조회
6. 메타데이터 저장/로드

---

### E-2: SQL Parser (쿼리 파싱)
```c
typedef enum {
    SQL_SELECT,
    SQL_INSERT,
    SQL_UPDATE,
    SQL_DELETE,
    SQL_CREATE
} SQLType;

typedef struct {
    SQLType type;
    char *table_name;
    Vector *columns;         // SELECT 컬럼들
    Vector *values;          // INSERT 값들
    String *where_clause;    // WHERE 조건
} SQLStatement;
```

**파일**:
```
src/sql_parser.h/c    (신규, ~400줄)
test_parser.c         (신규, ~250줄)
```

**테스트** (5 tests):
1. SELECT 파싱
2. INSERT 파싱
3. UPDATE 파싱
4. DELETE 파싱
5. 에러 처리

---

### E-3: Query Executor (쿼리 실행)
```c
typedef struct {
    Vector *tables;          // 로드된 테이블들
    String *error_msg;
} Database;

Database* db_new(void);
int db_create_table(Database *db, const char *name);
int db_insert(Database *db, const char *table, Vector *values);
Vector* db_select(Database *db, const char *table, String *where);
int db_update(Database *db, const char *table, Vector *values, String *where);
int db_delete(Database *db, const char *table, String *where);
```

**파일**:
```
src/sql_executor.h/c  (신규, ~500줄)
test_executor.c       (신규, ~300줄)
```

**테스트** (8 tests):
1. CREATE TABLE
2. INSERT
3. SELECT (전체)
4. SELECT (WHERE)
5. UPDATE
6. DELETE
7. 인덱스 조회
8. 에러 처리

---

## 📊 Test 계획 (19 tests)

| Phase | 내용 | 테스트 | 코드 |
|-------|------|--------|------|
| E-1 | Table Management | 6 | 300 |
| E-2 | SQL Parser | 5 | 400 |
| E-3 | Query Executor | 8 | 500 |
| **합계** | **SQL Engine** | **19** | **1,200** |

---

## 🔄 구현 순서

1. **E-1: Table 정의 및 관리**
   - table.c/h 구현
   - test_table.c 작성
   - 6개 테스트 통과

2. **E-2: SQL 파서**
   - sql_parser.c/h 구현
   - test_parser.c 작성
   - 5개 테스트 통과

3. **E-3: 쿼리 실행**
   - sql_executor.c/h 구현
   - test_executor.c 작성
   - 8개 테스트 통과

---

## 💾 파일 구조

```
myos-lib/
├── src/
│   ├── table.c/h          (신규, E-1)
│   ├── sql_parser.c/h     (신규, E-2)
│   ├── sql_executor.c/h   (신규, E-3)
│   └── ... (Phase A-D)
├── test_table.c           (신규)
├── test_parser.c          (신규)
├── test_executor.c        (신규)
└── Makefile               (수정)
```

---

## 🚀 완성 기준

- ✅ 19개 테스트 모두 통과
- ✅ 기본 SQL 5가지 (CREATE, INSERT, SELECT, UPDATE, DELETE)
- ✅ WHERE 조건 필터링
- ✅ 메모리 누수 없음
- ✅ 500KB 바이너리 이하

---

## 📈 누적 진행도

```
Phase A: Memory (67) ✅
Phase B-1: Vector (56) ✅
Phase B-2: HashMap (48) ✅
Phase B-3: String (61) ✅
Phase B-4: LinkedList (74) ✅
Phase C: Serializer (18) ✅
Phase D: Runtime VM (52) ✅
Phase E: SQL Engine (19) 🔨
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
총: 395 tests
5,867+ 줄 Zero-Dependency C
```

---

## 🎯 기술 선택

| 항목 | 선택 | 이유 |
|------|------|------|
| **파서** | 수동 파싱 | Zero-Dependency |
| **인덱스** | HashMap | O(1) 조회 |
| **저장** | Serializer | 기존 시스템 활용 |
| **타입** | INT/TEXT/REAL | 기본 타입 3가지 |

---

**다음**: E-1 Table Management 구현 시작
