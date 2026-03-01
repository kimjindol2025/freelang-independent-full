#include "router.h"
#include "http.h"
#include "database.h"
#include "auth.h"
#include "storage.h"
#include "logs.h"
#include "sql_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 핸들러 선언
static void handle_health(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_status(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_register(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_login(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_verify(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_users(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_storage_get(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_storage_set(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_storage_delete(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_logs_get(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_logs_add(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_info(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_version(int client_fd, HttpRequest *req, AppContext *ctx);
static void handle_sql_execute_wrapper(int client_fd, HttpRequest *req, AppContext *ctx);

// 라우트 구조체
typedef struct {
    const char *method;
    const char *path;
    RouteHandler handler;
    int auth_required;
} Route;

// 라우트 테이블 (16개 엔드포인트)
static Route routes[] = {
    // 공개 API
    {"GET", "/health", handle_health, 0},
    {"GET", "/api/status", handle_status, 0},
    {"GET", "/api/version", handle_version, 0},
    {"GET", "/api/info", handle_info, 0},

    // 인증 API
    {"POST", "/api/auth/register", handle_register, 0},
    {"POST", "/api/auth/login", handle_login, 0},

    // 인증 필요
    {"GET", "/api/auth/verify", handle_verify, 1},
    {"GET", "/api/users", handle_users, 1},

    // KV 저장소 API
    {"GET", "/api/storage", handle_storage_get, 1},
    {"POST", "/api/storage", handle_storage_set, 1},
    {"DELETE", "/api/storage", handle_storage_delete, 1},

    // 로그 API
    {"GET", "/api/logs", handle_logs_get, 1},
    {"POST", "/api/logs", handle_logs_add, 1},

    // SQL API (공개)
    {"POST", "/api/sql/execute", handle_sql_execute_wrapper, 0},

    {NULL, NULL, NULL, 0}  // sentinel
};

// 라우트 초기화
int router_init(void) {
    return 0;
}

// 라우트 정리
void router_cleanup(void) {
}

// SQL API 래퍼 함수
static void handle_sql_execute_wrapper(int client_fd, HttpRequest *req, AppContext *ctx) {
    handle_sql_execute(client_fd, req, ctx);
}

// 간단한 JSON 파서 헬퍼
static int json_get_string(const char *json, const char *key, char *out, size_t out_size) {
    if (!json || !key || !out) {
        return -1;
    }

    char search[256];
    snprintf(search, sizeof(search), "\"%s\":\"", key);

    char *start = strstr(json, search);
    if (!start) {
        return -1;
    }

    start += strlen(search);
    char *end = strchr(start, '"');
    if (!end) {
        return -1;
    }

    size_t len = (size_t)(end - start);
    if (len >= out_size) {
        len = out_size - 1;
    }

    strncpy(out, start, len);
    out[len] = '\0';

    return 0;
}

// 토큰 검증 헬퍼
static int verify_token(HttpRequest *req, AppContext *ctx, JwtPayload *out_payload) {
    // Authorization 헤더에서 토큰 추출

    if (strlen(req->auth_token) == 0) {
        return -1;
    }

    // JWT 검증
    int result = jwt_verify(req->auth_token, ctx->secret, out_payload);
    return result;
}

// ============ 핸들러 구현 ============

// GET /health
static void handle_health(int client_fd, HttpRequest *req, AppContext *ctx) {
    http_respond(client_fd, 200, "{\"status\":\"ok\"}");
}

// GET /api/status
static void handle_status(int client_fd, HttpRequest *req, AppContext *ctx) {
    http_respond(client_fd, 200,
                 "{\"status\":\"running\",\"version\":\"0.1.0\",\"language\":\"C\",\"phase\":3}");
}

// POST /api/auth/register
static void handle_register(int client_fd, HttpRequest *req, AppContext *ctx) {
    Database *db = (Database*)ctx->db;

    char username[64];
    char email[128];
    char password[128];

    // JSON 파싱
    if (json_get_string(req->body, "username", username, sizeof(username)) < 0 ||
        json_get_string(req->body, "email", email, sizeof(email)) < 0 ||
        json_get_string(req->body, "password", password, sizeof(password)) < 0) {
        http_respond(client_fd, 400,
                     "{\"error\":\"Missing required fields\"}");
        return;
    }

    // 비밀번호 해싱
    char password_hash[65];
    hash_password(password, ctx->secret, password_hash);

    // 트랜잭션 시작
    if (db_transaction_begin(db) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Database error\"}");
        return;
    }

    // 사용자 삽입
    if (db_insert_user(db, username, email, password_hash, "user") < 0) {
        db_transaction_rollback(db);
        http_respond(client_fd, 400,
                     "{\"error\":\"Username or email already exists\"}");
        return;
    }

    // 커밋
    if (db_transaction_commit(db) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Database error\"}");
        return;
    }

    http_respond(client_fd, 201,
                 "{\"success\":true,\"message\":\"User registered\"}");
}

// POST /api/auth/login
static void handle_login(int client_fd, HttpRequest *req, AppContext *ctx) {
    Database *db = (Database*)ctx->db;

    char username[64];
    char password[128];

    // JSON 파싱
    if (json_get_string(req->body, "username", username, sizeof(username)) < 0 ||
        json_get_string(req->body, "password", password, sizeof(password)) < 0) {
        http_respond(client_fd, 400,
                     "{\"error\":\"Missing required fields\"}");
        return;
    }

    // 사용자 조회
    User user;
    if (db_get_user_by_username(db, username, &user) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid credentials\"}");
        return;
    }

    // 비밀번호 검증
    char password_hash[65];
    hash_password(password, ctx->secret, password_hash);

    if (strcmp(user.password_hash, password_hash) != 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid credentials\"}");
        return;
    }

    // JWT 생성
    char token[JWT_MAX_TOKEN_LEN];
    if (jwt_create(user.username, user.role, ctx->secret, 3600, token, sizeof(token)) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Token generation failed\"}");
        return;
    }

    // 응답
    char response[1024];
    snprintf(response, sizeof(response),
             "{\"success\":true,\"token\":\"%s\",\"user\":{\"username\":\"%s\",\"role\":\"%s\"}}",
             token, user.username, user.role);

    http_respond(client_fd, 200, response);
}

// GET /api/auth/verify (인증 필요)
static void handle_verify(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    // 응답
    char response[512];
    snprintf(response, sizeof(response),
             "{\"valid\":true,\"username\":\"%s\",\"role\":\"%s\",\"exp\":%ld}",
             payload.username, payload.role, payload.exp);

    http_respond(client_fd, 200, response);
}

// GET /api/users (인증 필요)
static void handle_users(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    // 관리자 확인 (간단한 구현)
    if (strcmp(payload.role, "admin") != 0) {
        http_respond(client_fd, 403,
                     "{\"error\":\"Admin access required\"}");
        return;
    }

    // 사용자 목록 (간단히 로그인한 사용자만)
    char response[512];
    snprintf(response, sizeof(response),
             "{\"users\":[{\"username\":\"%s\",\"role\":\"%s\"}]}",
             payload.username, payload.role);

    http_respond(client_fd, 200, response);
}

// GET /api/storage (KV 저장소 조회)
static void handle_storage_get(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    Storage *storage = (Storage*)ctx->storage;
    if (!storage) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Storage not available\"}");
        return;
    }

    // 저장소 목록 (JSON)
    char json_list[4096];
    if (storage_list(storage, json_list, sizeof(json_list)) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Storage list failed\"}");
        return;
    }

    char response[4096];
    snprintf(response, sizeof(response),
             "{\"success\":true,\"items\":%s}",
             json_list);

    http_respond(client_fd, 200, response);
}

// POST /api/storage (KV 저장소 저장)
static void handle_storage_set(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    char key[128];
    char value[512];

    // JSON 파싱
    if (json_get_string(req->body, "key", key, sizeof(key)) < 0 ||
        json_get_string(req->body, "value", value, sizeof(value)) < 0) {
        http_respond(client_fd, 400,
                     "{\"error\":\"Missing key or value\"}");
        return;
    }

    Storage *storage = (Storage*)ctx->storage;
    if (!storage) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Storage not available\"}");
        return;
    }

    // 저장
    if (storage_set(storage, key, value) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Storage set failed\"}");
        return;
    }

    http_respond(client_fd, 201,
                 "{\"success\":true,\"message\":\"Item stored\"}");
}

// DELETE /api/storage (KV 저장소 삭제)
static void handle_storage_delete(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    char key[128];

    // JSON 파싱
    if (json_get_string(req->body, "key", key, sizeof(key)) < 0) {
        http_respond(client_fd, 400,
                     "{\"error\":\"Missing key\"}");
        return;
    }

    Storage *storage = (Storage*)ctx->storage;
    if (!storage) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Storage not available\"}");
        return;
    }

    // 삭제
    if (storage_delete(storage, key) < 0) {
        http_respond(client_fd, 404,
                     "{\"error\":\"Key not found\"}");
        return;
    }

    http_respond(client_fd, 200,
                 "{\"success\":true,\"message\":\"Item deleted\"}");
}

// GET /api/logs (로그 조회)
static void handle_logs_get(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    Logs *logs = (Logs*)ctx->logs;
    if (!logs) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Logs not available\"}");
        return;
    }

    // 로그 목록 (JSON)
    char json_list[8192];
    if (logs_list(logs, json_list, sizeof(json_list)) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Logs list failed\"}");
        return;
    }

    char response[8192];
    snprintf(response, sizeof(response),
             "{\"success\":true,\"logs\":%s}",
             json_list);

    http_respond(client_fd, 200, response);
}

// POST /api/logs (로그 저장)
static void handle_logs_add(int client_fd, HttpRequest *req, AppContext *ctx) {
    JwtPayload payload;

    // 토큰 검증
    if (verify_token(req, ctx, &payload) < 0) {
        http_respond(client_fd, 401,
                     "{\"error\":\"Invalid or expired token\"}");
        return;
    }

    char level[16];
    char message[256];

    // JSON 파싱
    if (json_get_string(req->body, "level", level, sizeof(level)) < 0 ||
        json_get_string(req->body, "message", message, sizeof(message)) < 0) {
        http_respond(client_fd, 400,
                     "{\"error\":\"Missing level or message\"}");
        return;
    }

    Logs *logs = (Logs*)ctx->logs;
    if (!logs) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Logs not available\"}");
        return;
    }

    // 로그 추가
    if (logs_add(logs, level, message, payload.username) < 0) {
        http_respond(client_fd, 500,
                     "{\"error\":\"Logs add failed\"}");
        return;
    }

    http_respond(client_fd, 201,
                 "{\"success\":true,\"message\":\"Log added\"}");
}

// GET /api/version (버전 정보)
static void handle_version(int client_fd, HttpRequest *req, AppContext *ctx) {
    http_respond(client_fd, 200,
                 "{\"version\":\"0.1.0\",\"language\":\"C\",\"phase\":4,\"api_endpoints\":15}");
}

// GET /api/info (서버 정보)
static void handle_info(int client_fd, HttpRequest *req, AppContext *ctx) {
    time_t now = time(NULL);
    char response[512];
    snprintf(response, sizeof(response),
             "{\"name\":\"FreeLang C Server\",\"version\":\"0.1.0\",\"timestamp\":%ld,\"features\":[\"JWT Auth\",\"KV Storage\",\"Logs\",\"RBAC\"]}",
             now);
    http_respond(client_fd, 200, response);
}

// ============ 라우터 메인 ============

// 라우트 매칭 및 실행
void router_handle(int client_fd, HttpRequest *req, AppContext *ctx) {
    for (int i = 0; routes[i].path; i++) {
        if (strcmp(routes[i].method, req->method) == 0 &&
            strcmp(routes[i].path, req->path) == 0) {

            // 인증 필요 확인
            if (routes[i].auth_required && strlen(req->auth_token) == 0) {
                http_respond(client_fd, 401, "{\"error\":\"Authentication required\"}");
                return;
            }

            routes[i].handler(client_fd, req, ctx);
            return;
        }
    }

    // 404
    http_respond(client_fd, 404, "{\"error\":\"Not Found\"}");
}
