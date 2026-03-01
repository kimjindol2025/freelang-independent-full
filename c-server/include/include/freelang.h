#ifndef FREELANG_H
#define FREELANG_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>

// 공통 상수
#define HTTP_MAX_METHOD    8
#define HTTP_MAX_PATH      256
#define HTTP_MAX_HEADER    512
#define HTTP_MAX_BODY      4096
#define HTTP_MAX_TOKEN     512

// HTTP 요청 구조체
typedef struct {
    char method[HTTP_MAX_METHOD];
    char path[HTTP_MAX_PATH];
    char body[HTTP_MAX_BODY];
    char auth_token[HTTP_MAX_TOKEN];
    int  content_length;
    int  status_code;
} HttpRequest;

// JSON 값 타입 (Phase 4에서 구현)
typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_INT,
    JSON_STRING,
    JSON_OBJECT,
    JSON_ARRAY
} JsonType;

// 응용 컨텍스트
typedef struct {
    void *db;          // SQLite3 핸들 (Phase 2)
    void *storage;     // KV 저장소 (Phase 3)
    void *logs;        // 로그 저장소 (Phase 4)
    char secret[64];   // JWT 서명 비밀키
} AppContext;

// 함수 포인터 라우트 핸들러 (Phase 4)
typedef void (*RouteHandler)(int client_fd, HttpRequest *req, AppContext *ctx);

#endif
