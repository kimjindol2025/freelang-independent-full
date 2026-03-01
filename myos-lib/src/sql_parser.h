#ifndef MYOS_SQL_PARSER_H
#define MYOS_SQL_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include "vector.h"
#include "string.h"

/* ============================================================================
 * SQL Parser - SQL 쿼리 파싱
 * ============================================================================ */

/* SQL 명령 타입 */
typedef enum {
    SQL_UNKNOWN = 0,
    SQL_SELECT = 1,
    SQL_INSERT = 2,
    SQL_UPDATE = 3,
    SQL_DELETE = 4,
    SQL_CREATE = 5
} SQLType;

/* SQL 파싱된 문장 */
typedef struct {
    SQLType type;                   /* 명령 타입 */
    String *table_name;             /* 테이블명 */
    Vector *columns;                /* 컬럼 목록 (String*) */
    Vector *values;                 /* 값 목록 (String*) */
    String *where_clause;           /* WHERE 조건 */
    String *error_msg;              /* 에러 메시지 */
} SQLStatement;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * SQL 쿼리 파싱
 * @param sql_query: SQL 쿼리 문자열 (예: "SELECT id, name FROM users WHERE id = 1")
 * @return: 파싱된 SQLStatement 또는 NULL
 */
SQLStatement* sql_parse(const char *sql_query);

/**
 * SQLStatement 해제
 */
void sql_statement_free(SQLStatement *stmt);

/**
 * 파싱된 명령 타입 확인
 */
SQLType sql_get_type(SQLStatement *stmt);

/**
 * 테이블명 조회
 */
const char* sql_get_table(SQLStatement *stmt);

/**
 * 컬럼 개수 조회
 */
size_t sql_column_count(SQLStatement *stmt);

/**
 * 특정 컬럼명 조회
 */
const char* sql_get_column(SQLStatement *stmt, size_t index);

/**
 * 값 개수 조회
 */
size_t sql_value_count(SQLStatement *stmt);

/**
 * 특정 값 조회
 */
const char* sql_get_value(SQLStatement *stmt, size_t index);

/**
 * WHERE 조건 조회
 */
const char* sql_get_where(SQLStatement *stmt);

/**
 * 에러 메시지 조회
 */
const char* sql_get_error(SQLStatement *stmt);

#endif
