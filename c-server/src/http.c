#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CRLF "\r\n"

// HTTP 요청 라인 파싱 (예: "GET /path HTTP/1.1")
static int parse_request_line(char *line, HttpRequest *req) {
    // 간단한 파싱: 공백으로 분리
    char *space1 = strchr(line, ' ');
    if (!space1) {
        return -1;
    }

    char *space2 = strchr(space1 + 1, ' ');
    if (!space2) {
        return -1;
    }

    // 메소드
    int method_len = (int)(space1 - line);
    if (method_len >= HTTP_MAX_METHOD) {
        return -1;
    }
    strncpy(req->method, line, method_len);
    req->method[method_len] = '\0';

    // 경로
    int path_len = (int)(space2 - space1 - 1);
    if (path_len >= HTTP_MAX_PATH) {
        return -1;
    }
    strncpy(req->path, space1 + 1, path_len);
    req->path[path_len] = '\0';

    return 0;
}

// HTTP 헤더 파싱
static int parse_headers(char *buffer, HttpRequest *req) {
    char *pos = buffer;

    // 첫 줄: 요청 라인
    char *eol = strstr(pos, CRLF);
    if (!eol) {
        return -1;
    }

    char request_line[512];
    int req_line_len = (int)(eol - pos);
    strncpy(request_line, pos, req_line_len);
    request_line[req_line_len] = '\0';

    if (parse_request_line(request_line, req) < 0) {
        return -1;
    }

    printf("[DEBUG] Parsing headers...\n");
    fflush(stdout);

    // 나머지 헤더들
    pos = eol + 2;  // CRLF 건너뛰기

    int header_count = 0;
    while (*pos) {
        eol = strstr(pos, CRLF);
        if (!eol) {
            break;
        }

        if (eol == pos) {
            // 빈 줄 (헤더 끝)
            printf("[DEBUG] Headers end. Total headers: %d\n", header_count);
            fflush(stdout);
            break;
        }

        int line_len = (int)(eol - pos);
        char line[512];
        strncpy(line, pos, line_len);
        line[line_len] = '\0';
        printf("[DEBUG] Header line: %s\n", line);
        fflush(stdout);
        header_count++;

        // Content-Length 파싱
        if (strncmp(line, "Content-Length:", 15) == 0) {
            req->content_length = atoi(line + 15);
        }

        // Authorization 헤더 파싱
        if (strncmp(line, "Authorization:", 14) == 0) {
            char *token_start = line + 14;
            while (*token_start == ' ') token_start++;

            printf("[DEBUG] Authorization header found\n");
            fflush(stdout);

            // "Bearer <token>" 형식 처리
            if (strncmp(token_start, "Bearer ", 7) == 0) {
                char *bearer_token = token_start + 7;
                int token_len = 0;
                // 토큰 끝 찾기 (공백이나 null 종료까지)
                while (bearer_token[token_len] && bearer_token[token_len] != ' ' &&
                       bearer_token[token_len] != '\r' && bearer_token[token_len] != '\n') {
                    token_len++;
                }
                if (token_len > 0 && token_len < HTTP_MAX_TOKEN) {
                    strncpy(req->auth_token, bearer_token, token_len);
                    req->auth_token[token_len] = '\0';
                    printf("[DEBUG] Token stored, len=%d\n", token_len);
                    fflush(stdout);
                }
            }
        }

        pos = eol + 2;
    }

    return 0;
}

// HTTP 요청 파싱
int http_parse(int client_fd, HttpRequest *req) {
    // ⭐ 중요: 구조체 초기화
    memset(req, 0, sizeof(HttpRequest));

    printf("[DEBUG] http_parse called\n");
    fflush(stdout);

    char buffer[HTTP_MAX_HEADER + HTTP_MAX_BODY];
    int total_received = 0;

    // 헤더 + 바디 수신
    int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        return -1;
    }

    buffer[received] = '\0';
    total_received = received;

    // 빈 줄(CRLFCRLF)까지 헤더 파싱
    char *header_end = strstr(buffer, CRLF CRLF);
    if (!header_end) {
        return -1;
    }

    // 헤더 부분 복사 (원본 보존)
    char header_buffer[HTTP_MAX_HEADER];
    int header_len = (int)(header_end - buffer);
    strncpy(header_buffer, buffer, header_len);
    header_buffer[header_len] = '\0';

    // 헤더 파싱
    if (parse_headers(header_buffer, req) < 0) {
        return -1;
    }


    // 바디 복사
    char *body_start = header_end + 4;  // CRLFCRLF 스킵

    // 실제 바디 길이 계산
    int actual_body_len = received - (int)(body_start - buffer);

    // Content-Length가 없으면 실제 바디 길이 사용
    if (req->content_length == 0 && actual_body_len > 0) {
        req->content_length = actual_body_len;
    }

    if (actual_body_len > 0) {
        int body_len = actual_body_len;
        if (body_len > HTTP_MAX_BODY - 1) {
            body_len = HTTP_MAX_BODY - 1;
        }
        strncpy(req->body, body_start, body_len);
        req->body[body_len] = '\0';
    }

    return 0;
}

// 상태 메시지
const char *http_status_text(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        default:  return "Unknown";
    }
}

// JSON 응답 전송
int http_respond(int client_fd, int status, const char *json) {
    char response[8192];
    int json_len = strlen(json);

    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s" CRLF
             "Content-Type: application/json" CRLF
             "Content-Length: %d" CRLF
             "Connection: close" CRLF
             CRLF
             "%s",
             status, http_status_text(status),
             json_len, json);

    send(client_fd, response, strlen(response), 0);
    return 0;
}

// 일반 텍스트 응답 전송
int http_respond_string(int client_fd, int status, const char *body) {
    char response[8192];
    int body_len = strlen(body);

    snprintf(response, sizeof(response),
             "HTTP/1.1 %d %s" CRLF
             "Content-Type: text/plain" CRLF
             "Content-Length: %d" CRLF
             "Connection: close" CRLF
             CRLF
             "%s",
             status, http_status_text(status),
             body_len, body);

    send(client_fd, response, strlen(response), 0);
    return 0;
}
