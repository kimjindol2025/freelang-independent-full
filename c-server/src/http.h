#ifndef HTTP_H
#define HTTP_H

#include "../include/freelang.h"

// HTTP 요청 파싱
int http_parse(int client_fd, HttpRequest *req);

// HTTP 응답 전송
int http_respond(int client_fd, int status, const char *json);
int http_respond_string(int client_fd, int status, const char *body);

// 상태 코드 메시지
const char *http_status_text(int code);

#endif
