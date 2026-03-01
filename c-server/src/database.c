#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 데이터베이스 초기화
int db_init(const char *path, Database *out_db) {
    int rc;

    // SQLite 열기
    rc = sqlite3_open(path, &out_db->db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(out_db->db));
        return -1;
    }

    // WAL 모드 활성화 (동시 읽기 허용)
    sqlite3_exec(out_db->db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);

    // 타임아웃 설정
    sqlite3_busy_timeout(out_db->db, 5000);  // 5초

    // 테이블 생성
    const char *sql_users =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT UNIQUE NOT NULL,"
        "  email TEXT UNIQUE NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  role TEXT DEFAULT 'user',"
        "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ");";

    rc = sqlite3_exec(out_db->db, sql_users, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create users table: %s\n",
                sqlite3_errmsg(out_db->db));
        sqlite3_close(out_db->db);
        return -1;
    }

    // Prepared Statements 준비
    const char *sql_insert =
        "INSERT INTO users (username, email, password_hash, role) "
        "VALUES (?, ?, ?, ?);";

    rc = sqlite3_prepare_v2(out_db->db, sql_insert, -1,
                            &out_db->stmt_insert_user, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare insert statement: %s\n",
                sqlite3_errmsg(out_db->db));
        sqlite3_close(out_db->db);
        return -1;
    }

    const char *sql_get =
        "SELECT id, username, email, password_hash, role FROM users "
        "WHERE username = ? LIMIT 1;";

    rc = sqlite3_prepare_v2(out_db->db, sql_get, -1,
                            &out_db->stmt_get_user, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare get user statement: %s\n",
                sqlite3_errmsg(out_db->db));
        sqlite3_finalize(out_db->stmt_insert_user);
        sqlite3_close(out_db->db);
        return -1;
    }

    const char *sql_get_email =
        "SELECT id, username, email, password_hash, role FROM users "
        "WHERE email = ? LIMIT 1;";

    rc = sqlite3_prepare_v2(out_db->db, sql_get_email, -1,
                            &out_db->stmt_get_user_by_email, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare get user by email statement: %s\n",
                sqlite3_errmsg(out_db->db));
        sqlite3_finalize(out_db->stmt_insert_user);
        sqlite3_finalize(out_db->stmt_get_user);
        sqlite3_close(out_db->db);
        return -1;
    }

    printf("Database initialized: %s\n", path);
    return 0;
}

// 데이터베이스 종료
int db_close(Database *db) {
    if (db->stmt_insert_user) sqlite3_finalize(db->stmt_insert_user);
    if (db->stmt_get_user) sqlite3_finalize(db->stmt_get_user);
    if (db->stmt_get_user_by_email) sqlite3_finalize(db->stmt_get_user_by_email);
    if (db->db) sqlite3_close(db->db);

    return 0;
}

// 트랜잭션 시작
int db_begin(Database *db) {
    int rc = sqlite3_exec(db->db, "BEGIN TRANSACTION;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "BEGIN failed: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }
    return 0;
}

// 트랜잭션 커밋
int db_commit(Database *db) {
    int rc = sqlite3_exec(db->db, "COMMIT;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "COMMIT failed: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }
    return 0;
}

// 트랜잭션 롤백
int db_rollback(Database *db) {
    int rc = sqlite3_exec(db->db, "ROLLBACK;", NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "ROLLBACK failed: %s\n", sqlite3_errmsg(db->db));
        return -1;
    }
    return 0;
}

// 사용자 삽입
int db_insert_user(Database *db, const char *username, const char *email,
                   const char *password_hash, const char *role) {
    int rc;

    // 바인딩
    sqlite3_bind_text(db->stmt_insert_user, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(db->stmt_insert_user, 2, email, -1, SQLITE_STATIC);
    sqlite3_bind_text(db->stmt_insert_user, 3, password_hash, -1, SQLITE_STATIC);
    sqlite3_bind_text(db->stmt_insert_user, 4, role, -1, SQLITE_STATIC);

    // 실행
    rc = sqlite3_step(db->stmt_insert_user);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "INSERT failed: %s\n", sqlite3_errmsg(db->db));
        sqlite3_reset(db->stmt_insert_user);
        return -1;
    }

    // 리셋
    sqlite3_reset(db->stmt_insert_user);
    return 0;
}

// 사용자명으로 사용자 조회
int db_get_user_by_username(Database *db, const char *username, User *out_user) {
    int rc;

    sqlite3_bind_text(db->stmt_get_user, 1, username, -1, SQLITE_STATIC);

    rc = sqlite3_step(db->stmt_get_user);
    if (rc == SQLITE_ROW) {
        out_user->id = sqlite3_column_int(db->stmt_get_user, 0);
        strncpy(out_user->username,
                (const char*)sqlite3_column_text(db->stmt_get_user, 1), 63);
        strncpy(out_user->email,
                (const char*)sqlite3_column_text(db->stmt_get_user, 2), 127);
        strncpy(out_user->password_hash,
                (const char*)sqlite3_column_text(db->stmt_get_user, 3), 64);
        strncpy(out_user->role,
                (const char*)sqlite3_column_text(db->stmt_get_user, 4), 31);

        sqlite3_reset(db->stmt_get_user);
        return 0;
    }

    sqlite3_reset(db->stmt_get_user);
    return -1;  // 사용자 없음
}

// 이메일로 사용자 조회
int db_get_user_by_email(Database *db, const char *email, User *out_user) {
    int rc;

    sqlite3_bind_text(db->stmt_get_user_by_email, 1, email, -1, SQLITE_STATIC);

    rc = sqlite3_step(db->stmt_get_user_by_email);
    if (rc == SQLITE_ROW) {
        out_user->id = sqlite3_column_int(db->stmt_get_user_by_email, 0);
        strncpy(out_user->username,
                (const char*)sqlite3_column_text(db->stmt_get_user_by_email, 1), 63);
        strncpy(out_user->email,
                (const char*)sqlite3_column_text(db->stmt_get_user_by_email, 2), 127);
        strncpy(out_user->password_hash,
                (const char*)sqlite3_column_text(db->stmt_get_user_by_email, 3), 64);
        strncpy(out_user->role,
                (const char*)sqlite3_column_text(db->stmt_get_user_by_email, 4), 31);

        sqlite3_reset(db->stmt_get_user_by_email);
        return 0;
    }

    sqlite3_reset(db->stmt_get_user_by_email);
    return -1;  // 사용자 없음
}
