#ifndef ROUTER_H
#define ROUTER_H

#include "../include/freelang.h"

// 라우트 등록 및 실행
int router_init(void);
void router_handle(int client_fd, HttpRequest *req, AppContext *ctx);
void router_cleanup(void);

#endif
