#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 메모리 기반 사용자 저장소 (최대 1000명) */
#define MAX_USERS 1000

typedef struct {
    int id;
    char username[128];
    char email[256];
    char password_hash[65];
    char role[32];
    char created_at[32];
} StoredUser;

static StoredUser users[MAX_USERS];
static int user_count = 0;
static int next_user_id = 1;

/* 데이터베이스 초기화 */
int db_init(const char *path, Database *out_db) {
    out_db->db = NULL;  /* 사용 안 함 */
    user_count = 0;
    next_user_id = 1;

    /* 기본 사용자 추가 */
    strcpy(users[0].username, "admin");
    strcpy(users[0].email, "admin@freelang.dev");
    strcpy(users[0].password_hash,
           "701818837884979ce5ffc542ffe469cea0958f1adbafc83f3e808dc5d5848cbb");  /* SHA256("admin" + "freelang-secret-key-2026") */
    strcpy(users[0].role, "admin");
    strcpy(users[0].created_at, "2026-03-01T00:00:00Z");
    users[0].id = 1;

    user_count = 1;
    next_user_id = 2;

    return 0;
}

/* 데이터베이스 닫기 */
void db_close(Database *db) {
    if (!db) return;
    /* 메모리 기반이므로 특별히 할 것 없음 */
}

/* 사용자 삽입 */
int db_insert_user(Database *db, const char *username, const char *email,
                   const char *password_hash, const char *role) {
    if (!db || user_count >= MAX_USERS) return -1;

    StoredUser *u = &users[user_count];
    u->id = next_user_id++;
    strncpy(u->username, username, sizeof(u->username) - 1);
    strncpy(u->email, email, sizeof(u->email) - 1);
    strncpy(u->password_hash, password_hash, sizeof(u->password_hash) - 1);
    strncpy(u->role, role ? role : "user", sizeof(u->role) - 1);
    strcpy(u->created_at, "2026-03-01T00:00:00Z");

    user_count++;
    return 0;
}

/* 사용자명으로 조회 */
int db_get_user_by_username(Database *db, const char *username, User *out_user) {
    if (!db || !username || !out_user) return -1;

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            out_user->id = users[i].id;
            strcpy(out_user->username, users[i].username);
            strcpy(out_user->email, users[i].email);
            strcpy(out_user->password_hash, users[i].password_hash);
            strcpy(out_user->role, users[i].role);
            strcpy(out_user->created_at, users[i].created_at);
            return 0;
        }
    }

    return -1;  /* 찾지 못함 */
}

/* 이메일로 조회 */
int db_get_user_by_email(Database *db, const char *email, User *out_user) {
    if (!db || !email || !out_user) return -1;

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].email, email) == 0) {
            out_user->id = users[i].id;
            strcpy(out_user->username, users[i].username);
            strcpy(out_user->email, users[i].email);
            strcpy(out_user->password_hash, users[i].password_hash);
            strcpy(out_user->role, users[i].role);
            strcpy(out_user->created_at, users[i].created_at);
            return 0;
        }
    }

    return -1;  /* 찾지 못함 */
}

/* 트랜잭션 시작 */
int db_transaction_begin(Database *db) {
    if (!db) return -1;
    return 0;  /* 메모리 기반이므로 특별히 할 것 없음 */
}

/* 트랜잭션 커밋 */
int db_transaction_commit(Database *db) {
    if (!db) return -1;
    return 0;  /* 메모리 기반이므로 특별히 할 것 없음 */
}

/* 트랜잭션 롤백 */
int db_transaction_rollback(Database *db) {
    if (!db) return -1;
    return 0;  /* 메모리 기반이므로 특별히 할 것 없음 */
}

/* 모든 사용자 조회 */
int db_get_all_users(Database *db, User *out_users, int max_count, int *out_count) {
    if (!db || !out_users || !out_count) return -1;

    *out_count = (user_count > max_count) ? max_count : user_count;

    for (int i = 0; i < *out_count; i++) {
        out_users[i].id = users[i].id;
        strcpy(out_users[i].username, users[i].username);
        strcpy(out_users[i].email, users[i].email);
        strcpy(out_users[i].password_hash, users[i].password_hash);
        strcpy(out_users[i].role, users[i].role);
        strcpy(out_users[i].created_at, users[i].created_at);
    }

    return 0;
}
