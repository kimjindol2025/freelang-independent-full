#include "sql_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* MyOS_Lib SQL Engine headers */
#include "mm.h"
#include "vector.h"
#include "string.h"
#include "table.h"
#include "sql_parser.h"
#include "sql_executor.h"
#include "sql_manager.h"

/* 글로벌 SQL 관리자 */
static SQLManager *g_sql_manager = NULL;

/* SQL 관리자 초기화 */
static int sql_api_init(void) {
    if (g_sql_manager == NULL) {
        g_sql_manager = sql_manager_new();
        if (g_sql_manager == NULL) {
            return -1;
        }
    }
    return 0;
}

/* JSON 응답 생성 */
static void send_json_response(int client_fd, int status, const char *message,
                               const char *data_json) {
    char body[2048];
    char response[4096];

    /* 먼저 JSON 본문 생성 */
    int body_len;
    if (data_json) {
        body_len = snprintf(body, sizeof(body),
                           "{\"status\":%d,\"message\":\"%s\",\"data\":%s}",
                           status, message, data_json);
    } else {
        body_len = snprintf(body, sizeof(body),
                           "{\"status\":%d,\"message\":\"%s\"}",
                           status, message);
    }

    if (body_len < 0) return;

    /* HTTP 헤더 + 본문 생성 */
    int len = snprintf(response, sizeof(response),
                      "HTTP/1.1 %d OK\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: %d\r\n"
                      "\r\n"
                      "%s",
                      status, body_len, body);

    if (len > 0) {
        write(client_fd, response, (size_t)len);
    }
}

/* 간단한 JSON 파서 - query 필드 추출 */
static int json_get_query(const char *json, char *out, size_t out_size) {
    if (!json || !out) return -1;

    const char *start = strstr(json, "\"query\":\"");
    if (!start) return -1;

    start += strlen("\"query\":\"");
    const char *end = strchr(start, '"');
    if (!end) return -1;

    size_t len = (size_t)(end - start);
    if (len >= out_size) len = out_size - 1;

    strncpy(out, start, len);
    out[len] = '\0';

    return 0;
}

/* SQL 쿼리 실행 핸들러 */
void handle_sql_execute(int client_fd, HttpRequest *req, AppContext *ctx) {
    fprintf(stderr, "[SQL API] handle_sql_execute called\n");
    fflush(stderr);

    if (!req) {
        fprintf(stderr, "[SQL API] req is NULL\n");
        fflush(stderr);
        send_json_response(client_fd, 400, "Bad request", NULL);
        return;
    }

    fprintf(stderr, "[SQL API] req->body: %s\n", req->body ? req->body : "(NULL)");
    fflush(stderr);

    if (!req->body || req->body[0] == '\0') {
        fprintf(stderr, "[SQL API] body is empty or NULL\n");
        fflush(stderr);
        send_json_response(client_fd, 400, "Bad request", NULL);
        return;
    }

    /* SQL 관리자 초기화 */
    fprintf(stderr, "[SQL API] Initializing SQL manager\n");
    fflush(stderr);
    if (sql_api_init() < 0) {
        fprintf(stderr, "[SQL API] sql_api_init failed\n");
        fflush(stderr);
        send_json_response(client_fd, 500, "SQL manager init failed", NULL);
        return;
    }
    fprintf(stderr, "[SQL API] SQL manager initialized\n");
    fflush(stderr);

    /* JSON에서 쿼리 추출 */
    char query[1024];
    fprintf(stderr, "[SQL API] Extracting query from JSON\n");
    if (json_get_query(req->body, query, sizeof(query)) < 0) {
        fprintf(stderr, "[SQL API] Failed to extract query from JSON: %s\n", req->body);
        send_json_response(client_fd, 400, "Missing 'query' field", NULL);
        return;
    }
    fprintf(stderr, "[SQL API] Query extracted: %s\n", query);

    /* SQL 파싱 */
    SQLStatement *stmt = sql_parse(query);
    if (!stmt) {
        send_json_response(client_fd, 400, "SQL parse error", NULL);
        return;
    }

    /* SQL 타입에 따라 처리 */
    SQLType type = sql_get_type(stmt);
    const char *table_name = sql_get_table(stmt);

    if (type == SQL_CREATE) {
        /* CREATE TABLE */
        if (!table_name) {
            sql_statement_free(stmt);
            send_json_response(client_fd, 400, "Missing table name", NULL);
            return;
        }

        Table *table = table_new(table_name);
        if (!table) {
            sql_statement_free(stmt);
            send_json_response(client_fd, 500, "Failed to create table", NULL);
            return;
        }

        /* 컬럼 추가 */
        for (size_t i = 0; i < sql_column_count(stmt); i++) {
            const char *col_name = sql_get_column(stmt, i);
            if (col_name) {
                /* 간단하게: 모든 컬럼을 TEXT로 생성 */
                table_add_column(table, col_name, COLUMN_TYPE_TEXT);
            }
        }

        /* 관리자에 추가 */
        if (sql_manager_add_table(g_sql_manager, table) < 0) {
            table_free(table);
            sql_statement_free(stmt);
            send_json_response(client_fd, 409, "Table already exists", NULL);
            return;
        }

        sql_statement_free(stmt);
        send_json_response(client_fd, 201, "Table created", NULL);
        return;
    }

    /* 다른 쿼리는 테이블이 필요 */
    if (!table_name) {
        sql_statement_free(stmt);
        send_json_response(client_fd, 400, "Missing table name", NULL);
        return;
    }

    Table *table = sql_manager_get_table(g_sql_manager, table_name);
    if (!table && type != SQL_CREATE) {
        sql_statement_free(stmt);
        send_json_response(client_fd, 404, "Table not found", NULL);
        return;
    }

    /* SQL 실행 */
    QueryResult *result = execute(table, stmt);
    if (!result) {
        sql_statement_free(stmt);
        send_json_response(client_fd, 500, "Execution error", NULL);
        return;
    }

    /* 응답 생성 */
    char response_data[2048];
    int len = snprintf(response_data, sizeof(response_data),
                       "{\"result\":\"%s\",\"affected_rows\":%zu,\"result_rows\":%zu}",
                       (result->result == EXEC_OK) ? "OK" : "ERROR",
                       result->affected_rows, result->result_rows);

    if (result->result == EXEC_OK) {
        send_json_response(client_fd, 200, "Query executed", response_data);
    } else {
        send_json_response(client_fd, 400, "Query failed", response_data);
    }

    query_result_free(result);
    sql_statement_free(stmt);
}
