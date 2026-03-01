#ifndef SERVER_H
#define SERVER_H

#include "../include/freelang.h"

// TCP 서버 초기화 및 실행
int server_init(int port);
int server_run(int server_fd);
void server_shutdown(int server_fd);

// 클라이언트 처리
int handle_client(int client_fd);

// 컨텍스트 설정
void server_set_context(AppContext *ctx);

#endif
