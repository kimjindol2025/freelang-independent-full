#include "server.h"
#include "http.h"
#include "router.h"
#include "../include/freelang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define LISTEN_BACKLOG 128
#define RECV_BUFFER_SIZE 8192

// 응용 컨텍스트 (전역)
static AppContext g_app_ctx;

// TCP 서버 초기화
int server_init(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        return -1;
    }

    // SO_REUSEADDR 설정 (포트 재사용 허용)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() failed");
        close(server_fd);
        return -1;
    }

    // 바인드
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;  // 모든 인터페이스
    addr.sin_port = htons(port);        // 네트워크 바이트 오더 (big-endian)

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind() failed");
        close(server_fd);
        return -1;
    }

    // 리스닝
    if (listen(server_fd, LISTEN_BACKLOG) < 0) {
        perror("listen() failed");
        close(server_fd);
        return -1;
    }

    printf("Server listening on 0.0.0.0:%d\n", port);
    return server_fd;
}

// 클라이언트 요청 처리
int handle_client(int client_fd) {
    HttpRequest req;
    memset(&req, 0, sizeof(req));

    // HTTP 요청 파싱
    if (http_parse(client_fd, &req) < 0) {
        const char *err_resp = "HTTP/1.1 400 Bad Request\r\n"
                               "Content-Type: application/json\r\n"
                               "Content-Length: 32\r\n"
                               "Connection: close\r\n"
                               "\r\n"
                               "{\"error\":\"Bad Request\"}";
        send(client_fd, err_resp, strlen(err_resp), 0);
        return -1;
    }

    // 라우터로 요청 처리
    router_handle(client_fd, &req, &g_app_ctx);

    return 0;
}

// 서버 실행 루프
int server_run(int server_fd) {
    printf("Starting server loop...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // 클라이언트 연결 수락
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept() failed");
            continue;
        }

        // 클라이언트 주소 로그
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // 요청 처리 (동기식)
        handle_client(client_fd);

        // 연결 종료
        close(client_fd);
    }

    return 0;
}

// 서버 종료
void server_shutdown(int server_fd) {
    router_cleanup();
    close(server_fd);
}

// 컨텍스트 설정
void server_set_context(AppContext *ctx) {
    memcpy(&g_app_ctx, ctx, sizeof(AppContext));
}
