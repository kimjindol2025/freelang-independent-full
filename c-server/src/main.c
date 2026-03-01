#include "server.h"
#include "database.h"
#include "router.h"
#include "storage.h"
#include "logs.h"
#include "../include/freelang.h"
/* MyOS_Lib 통합 */
#include "mm.h"
#include "vector.h"
#include "hash.h"
#include "string.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>

#define DEFAULT_PORT 40999
#define DB_PATH "data/freelang.db"
#define JWT_SECRET "freelang-secret-key-2026"

static int g_server_fd = -1;
static Database g_db;
static Storage *g_storage = NULL;
static Logs *g_logs = NULL;

// Ctrl+C 처리
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nShutting down server...\n");
        if (g_server_fd >= 0) {
            server_shutdown(g_server_fd);
        }
        db_close(&g_db);
        if (g_storage) {
            storage_free(g_storage);
        }
        if (g_logs) {
            logs_free(g_logs);
        }
        exit(0);
    }
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;

    // 명령행 인수 처리
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    printf("FreeLang C Server - Phase 4: Application Layer (+ MyOS_Lib Integration)\n");
    printf("Starting server on port %d...\n\n", port);

    /* Note: MyOS_Lib은 순수 C 라이브러리로 개별 함수들만 사용 (Hash, Vector, etc)
     * 메모리 할당은 libc malloc/free 사용 (mm_alloc은 선택적) */

    // 신호 핸들러 등록
    signal(SIGINT, signal_handler);

    // data 디렉토리 생성
    mkdir("data", 0755);

    // 데이터베이스 초기화
    if (db_init(DB_PATH, &g_db) < 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    // KV 저장소 초기화
    g_storage = storage_new();
    if (!g_storage) {
        fprintf(stderr, "Failed to initialize storage\n");
        db_close(&g_db);
        return 1;
    }

    // 로그 저장소 초기화
    g_logs = logs_new();
    if (!g_logs) {
        fprintf(stderr, "Failed to initialize logs\n");
        db_close(&g_db);
        storage_free(g_storage);
        return 1;
    }

    // 라우터 초기화
    router_init();

    // 응용 컨텍스트 구성
    AppContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.db = (void*)&g_db;
    ctx.storage = (void*)g_storage;
    ctx.logs = (void*)g_logs;
    strncpy(ctx.secret, JWT_SECRET, sizeof(ctx.secret) - 1);

    // 서버에 컨텍스트 설정
    server_set_context(&ctx);

    // 서버 초기화
    g_server_fd = server_init(port);
    if (g_server_fd < 0) {
        fprintf(stderr, "Failed to initialize server\n");
        db_close(&g_db);
        return 1;
    }

    // 서버 실행
    server_run(g_server_fd);

    // 정상 종료
    server_shutdown(g_server_fd);
    db_close(&g_db);
    if (g_storage) {
        storage_free(g_storage);
    }
    if (g_logs) {
        logs_free(g_logs);
    }
    return 0;
}
