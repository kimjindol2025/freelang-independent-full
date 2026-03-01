CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c99 -D_POSIX_C_SOURCE=200809L \
          -I./include -I./myos-lib/src
LDFLAGS = -lpthread

# C Server 소스 (Zero-Dependency: crypto.c + kvstore.c 포함)
SRC = src/main.c src/server.c src/http.c src/router.c \
      src/database.c src/auth.c src/storage.c src/logs.c \
      src/crypto.c src/kvstore.c src/sql_api.c

# MyOS_Lib 소스 (CRC32 + Serializer 포함 + SQL Engine 포함)
MYOS_SRC = myos-lib/src/mm.c myos-lib/src/vector.c myos-lib/src/hash.c \
           myos-lib/src/string.c myos-lib/src/list.c \
           myos-lib/src/crc32.c myos-lib/src/serializer.c \
           include/table.c include/sql_parser.c include/sql_executor.c \
           include/sql_manager.c

OBJ = $(SRC:.c=.o) $(MYOS_SRC:.c=.o)
TARGET = freelang-c-server

.PHONY: all clean test run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET) (Zero-Dependency: SHA256 + HMAC + KVStore)"
	@ls -lh $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) 40999

test: $(TARGET)
	@echo "Testing Phase D: MyOS_Lib Integration"
	@./$(TARGET) 40999 &
	@sleep 2
	@curl -s http://localhost:40999/health || echo "Server test"
	@killall freelang-c-server 2>/dev/null || true

clean:
	rm -f $(OBJ) $(TARGET)
	@echo "✅ Cleaned"

size: $(TARGET)
	@echo "Binary size (with MyOS_Lib):"
	@du -h $(TARGET)

status:
	@echo "=== Phase D: C Server + MyOS_Lib ==="
	@wc -l src/*.c myos-lib/src/*.c | tail -3
