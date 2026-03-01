CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2
CFLAGS_LIB = -nostdlib
LDFLAGS =

# 소스 파일
SRC = src/mm.c src/vector.c src/hash.c src/string.c src/list.c src/serializer.c
OBJ = $(SRC:.c=.o)
HEADER = src/mm.h src/vector.h src/hash.h src/string.h src/list.h src/serializer.h

# 테스트
TEST_MM_SRC = test_mm.c
TEST_MM_OBJ = $(TEST_MM_SRC:.c=.o)
TEST_MM_BIN = test_mm

TEST_VEC_SRC = test_vector.c
TEST_VEC_OBJ = $(TEST_VEC_SRC:.c=.o)
TEST_VEC_BIN = test_vector

TEST_HASH_SRC = test_hash.c
TEST_HASH_OBJ = $(TEST_HASH_SRC:.c=.o)
TEST_HASH_BIN = test_hash

TEST_STR_SRC = test_string.c
TEST_STR_OBJ = $(TEST_STR_SRC:.c=.o)
TEST_STR_BIN = test_string

TEST_LIST_SRC = test_list.c
TEST_LIST_OBJ = $(TEST_LIST_SRC:.c=.o)
TEST_LIST_BIN = test_list

TEST_SER_SRC = test_serializer.c
TEST_SER_OBJ = $(TEST_SER_SRC:.c=.o)
TEST_SER_BIN = test_serializer

TEST_BINS = $(TEST_MM_BIN) $(TEST_VEC_BIN) $(TEST_HASH_BIN) $(TEST_STR_BIN) $(TEST_LIST_BIN) $(TEST_SER_BIN)

# 라이브러리
LIB = libmyos_mm.a

# 기본 타겟
.PHONY: all clean test test-mm test-vec test-hash test-str test-list test-ser debug size

all: $(LIB) $(TEST_BINS)

# 라이브러리 생성
$(LIB): $(OBJ)
	ar rcs $@ $^
	@echo "✅ Library created: $@"

# 오브젝트 파일
src/mm.o: src/mm.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

src/vector.o: src/vector.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

src/hash.o: src/hash.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

src/string.o: src/string.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

src/list.o: src/list.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

src/serializer.o: src/serializer.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

test_mm.o: test_mm.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test_vector.o: test_vector.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test_hash.o: test_hash.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test_string.o: test_string.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test_list.o: test_list.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

test_serializer.o: test_serializer.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

# 테스트 바이너리
$(TEST_MM_BIN): test_mm.o src/mm.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

$(TEST_VEC_BIN): test_vector.o src/mm.o src/vector.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

$(TEST_HASH_BIN): test_hash.o src/mm.o src/hash.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

$(TEST_STR_BIN): test_string.o src/mm.o src/string.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

$(TEST_LIST_BIN): test_list.o src/mm.o src/list.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

$(TEST_SER_BIN): test_serializer.o src/mm.o src/serializer.o src/crc32.o src/vector.o src/hash.o src/string.o src/list.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

# 테스트 실행
test: test-mm test-vec test-hash test-str test-list test-ser

test-mm: $(TEST_MM_BIN)
	./$(TEST_MM_BIN)

test-vec: $(TEST_VEC_BIN)
	./$(TEST_VEC_BIN)

test-hash: $(TEST_HASH_BIN)
	./$(TEST_HASH_BIN)

test-str: $(TEST_STR_BIN)
	./$(TEST_STR_BIN)

test-list: $(TEST_LIST_BIN)
	./$(TEST_LIST_BIN)

test-ser: $(TEST_SER_BIN)
	./$(TEST_SER_BIN)

clean:
	rm -f $(OBJ) $(TEST_MM_OBJ) $(TEST_VEC_OBJ) $(TEST_HASH_OBJ) $(TEST_STR_OBJ) $(TEST_LIST_OBJ) $(TEST_SER_OBJ) $(LIB) $(TEST_BINS)
	@echo "✅ Clean complete"

# 디버그 빌드
debug: CFLAGS += -g -DDEBUG
debug: clean all

# 사이즈 확인
size: all
	@size $(LIB)
	@size $(TEST_MM_BIN)
	@size $(TEST_VEC_BIN)
	@size $(TEST_HASH_BIN)
	@size $(TEST_STR_BIN)
	@size $(TEST_LIST_BIN)
	@size $(TEST_SER_BIN)
	@echo "Binary sizes:"
	@ls -lh $(TEST_BINS)

src/runtime.o: src/runtime.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

test_runtime.o: test_runtime.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

TEST_RUNTIME_BIN = test_runtime
$(TEST_RUNTIME_BIN): test_runtime.o src/mm.o src/runtime.o src/serializer.o src/crc32.o src/vector.o src/hash.o src/string.o src/list.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

test-runtime: $(TEST_RUNTIME_BIN)
	./$(TEST_RUNTIME_BIN)

src/snapshot.o: src/snapshot.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

test_snapshot.o: test_snapshot.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

TEST_SNAPSHOT_BIN = test_snapshot
$(TEST_SNAPSHOT_BIN): test_snapshot.o src/mm.o src/snapshot.o src/serializer.o src/crc32.o src/vector.o src/hash.o src/string.o src/list.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

test-snapshot: $(TEST_SNAPSHOT_BIN)
	./$(TEST_SNAPSHOT_BIN)

src/object_pool.o: src/object_pool.c
	$(CC) $(CFLAGS) $(CFLAGS_LIB) -Isrc -c $< -o $@

test_pool.o: test_pool.c
	$(CC) $(CFLAGS) -Isrc -c $< -o $@

TEST_POOL_BIN = test_pool
$(TEST_POOL_BIN): test_pool.o src/mm.o src/object_pool.o
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ Test binary created: $@"

test-pool: $(TEST_POOL_BIN)
	./$(TEST_POOL_BIN)

test: test-runtime test-snapshot test-pool

clean:
	rm -f $(OBJ) test_runtime.o test_snapshot.o test_pool.o $(LIB) $(TEST_RUNTIME_BIN) $(TEST_SNAPSHOT_BIN) $(TEST_POOL_BIN)
	@echo "✅ Clean complete"
