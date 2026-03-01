#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

// 데이터베이스 구조체
typedef struct {
    sqlite3 *db;
    sqlite3_stmt *stmt_insert_user;
    sqlite3_stmt *stmt_get_user;
    sqlite3_stmt *stmt_get_user_by_email;
    sqlite3_stmt *stmt_list_users;
} Database;

// 사용자 구조체
typedef struct {
    int id;
    char username[64];
    char email[128];
    char password_hash[65];  // SHA256 hex = 64 chars + null
    char role[32];
} User;

// 데이터베이스 함수
int db_init(const char *path, Database *out_db);
int db_close(Database *db);

// 트랜잭션
int db_begin(Database *db);
int db_commit(Database *db);
int db_rollback(Database *db);

// 사용자 관리
int db_insert_user(Database *db, const char *username, const char *email,
                   const char *password_hash, const char *role);
int db_get_user_by_username(Database *db, const char *username, User *out_user);
int db_get_user_by_email(Database *db, const char *email, User *out_user);

#endif
