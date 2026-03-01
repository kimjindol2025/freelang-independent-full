#ifndef DATABASE_H
#define DATABASE_H

/* Zero-Dependency 메모리 기반 데이터베이스 */

typedef struct {
    void *db;  /* 사용하지 않음 (호환성) */
} Database;

typedef struct {
    int id;
    char username[128];
    char email[256];
    char password_hash[65];  /* SHA256 hex = 64 chars + null */
    char role[32];
    char created_at[32];
} User;

/* 데이터베이스 초기화/종료 */
int db_init(const char *path, Database *out_db);
void db_close(Database *db);

/* 트랜잭션 */
int db_transaction_begin(Database *db);
int db_transaction_commit(Database *db);
int db_transaction_rollback(Database *db);

/* 사용자 관리 */
int db_insert_user(Database *db, const char *username, const char *email,
                   const char *password_hash, const char *role);
int db_get_user_by_username(Database *db, const char *username, User *out_user);
int db_get_user_by_email(Database *db, const char *email, User *out_user);
int db_get_all_users(Database *db, User *out_users, int max_count, int *out_count);

#endif
