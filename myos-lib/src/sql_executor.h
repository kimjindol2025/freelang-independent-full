#ifndef SQL_EXECUTOR_H
#define SQL_EXECUTOR_H

#include "sql_parser.h"
#include "table.h"
#include "string.h"

/* 실행 결과 코드 */
typedef enum {
    EXEC_OK,           /* 성공 */
    EXEC_ERROR,        /* 실행 오류 */
    EXEC_NO_TABLE,     /* 테이블 없음 */
    EXEC_INVALID_COL,  /* 잘못된 컬럼 */
    EXEC_TYPE_ERROR    /* 타입 오류 */
} ExecResult;

/* 쿼리 실행 결과 */
typedef struct {
    ExecResult result;
    size_t affected_rows;  /* INSERT/UPDATE/DELETE 영향받은 행 수 */
    size_t result_rows;    /* SELECT 결과 행 수 */
    String *error_msg;
} QueryResult;

/* SQL 실행 함수들 */
QueryResult* execute_select(Table *table, SQLStatement *stmt);
QueryResult* execute_insert(Table *table, SQLStatement *stmt);
QueryResult* execute_update(Table *table, SQLStatement *stmt);
QueryResult* execute_delete(Table *table, SQLStatement *stmt);
QueryResult* execute_create(SQLStatement *stmt);

/* 메인 실행 함수 */
QueryResult* execute(Table *table, SQLStatement *stmt);

/* 메모리 해제 */
void query_result_free(QueryResult *result);

#endif
