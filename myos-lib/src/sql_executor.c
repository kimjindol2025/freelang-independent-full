#include "sql_executor.h"
#include "mm.h"
#include <string.h>

static void* my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static int my_strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return 1;
        s1++; s2++;
    }
    return (*s1 != *s2) ? 1 : 0;
}

static size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static long parse_int(const char *str) {
    long result = 0;
    int negative = 0;
    if (*str == '-') {
        negative = 1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return negative ? -result : result;
}

static double parse_double(const char *str) {
    double result = 0.0;
    int negative = 0;
    if (*str == '-') {
        negative = 1;
        str++;
    }
    while (*str >= '0' && *str <= '9') {
        result = result * 10.0 + (*str - '0');
        str++;
    }
    if (*str == '.') {
        str++;
        double fraction = 0.1;
        while (*str >= '0' && *str <= '9') {
            result += fraction * (*str - '0');
            fraction *= 0.1;
            str++;
        }
    }
    return negative ? -result : result;
}

/* WHERE 절에서 컬럼명과 값 추출 (길이 정보 포함) */
typedef struct {
    const char *col_name;
    size_t col_len;
    const char *val_name;
    size_t val_len;
} WhereCondition;

static WhereCondition parse_where_condition(const char *where) {
    WhereCondition cond = {0, 0, 0, 0};
    if (!where) return cond;

    const char *pos = where;

    /* 공백 건너뛰기 */
    while (*pos == ' ' || *pos == '\t') pos++;

    /* 컬럼명 길이 계산 */
    cond.col_name = pos;
    size_t col_len = 0;
    while (pos[col_len] && pos[col_len] != ' ' && pos[col_len] != '\t'
           && pos[col_len] != '=') {
        col_len++;
    }
    if (col_len == 0) return cond;

    cond.col_len = col_len;
    pos += col_len;

    /* 공백 건너뛰기 */
    while (*pos == ' ' || *pos == '\t') pos++;

    /* '=' 확인 */
    if (*pos != '=') return cond;
    pos++;

    /* 공백 건너뛰기 */
    while (*pos == ' ' || *pos == '\t') pos++;

    /* 값 길이 계산 */
    cond.val_name = pos;
    size_t val_len = 0;
    while (pos[val_len] && pos[val_len] != ' ' && pos[val_len] != '\t'
           && pos[val_len] != ';' && pos[val_len] != ')') {
        val_len++;
    }

    cond.val_len = val_len;

    return cond;
}

/* 문자열 n개 비교 */
static int my_strncmp_len(const char *s1, size_t len1, const char *s2) {
    size_t len2 = my_strlen(s2);
    if (len1 != len2) return 1;

    for (size_t i = 0; i < len1; i++) {
        if (s1[i] != s2[i]) return 1;
    }
    return 0;
}

/* 행이 WHERE 조건을 만족하는지 확인 */
static int matches_where(Table *table, void *row, SQLStatement *stmt) {
    const char *where = sql_get_where(stmt);
    if (!where) return 1;  /* WHERE 절 없음 = 모든 행 매칭 */

    WhereCondition cond = parse_where_condition(where);
    if (cond.col_len == 0 || cond.val_len == 0) {
        return 0;
    }

    /* 컬럼명으로 컬럼 검색 (길이 정보 사용) */
    size_t col_count = table_column_count(table);
    int col_idx = -1;

    for (size_t i = 0; i < col_count; i++) {
        Column *col = table_get_column(table, i);
        if (col && my_strncmp_len(cond.col_name, cond.col_len, col->name) == 0) {
            col_idx = (int)i;
            break;
        }
    }

    if (col_idx < 0) return 0;

    /* 컬럼 정보 */
    Column *col = table_get_column(table, col_idx);
    if (!col) return 0;

    /* 행 데이터에서 값 추출 */
    unsigned char *row_bytes = (unsigned char *)row;

    /* 값을 문자열로 추출 (길이 정보 사용) */
    char val_str[256];
    if (cond.val_len >= 256) cond.val_len = 255;
    my_memcpy(val_str, cond.val_name, cond.val_len);
    val_str[cond.val_len] = '\0';

    switch (col->type) {
        case COLUMN_TYPE_INT: {
            int *int_val = (int *)(row_bytes + col->offset);
            long expected = parse_int(val_str);
            return (*int_val == (int)expected) ? 1 : 0;
        }
        case COLUMN_TYPE_TEXT: {
            char *text_val = (char *)(row_bytes + col->offset);
            return (my_strcmp(text_val, val_str) == 0) ? 1 : 0;
        }
        case COLUMN_TYPE_REAL: {
            double *double_val = (double *)(row_bytes + col->offset);
            double expected = parse_double(val_str);
            return (*double_val == expected) ? 1 : 0;
        }
    }

    return 0;
}

/* SELECT 실행 */
QueryResult* execute_select(Table *table, SQLStatement *stmt) {
    QueryResult *result = (QueryResult *)mm_alloc(sizeof(QueryResult));
    if (!result) return NULL;

    result->error_msg = string_new_with_capacity(128);
    if (!result->error_msg) {
        mm_free(result);
        return NULL;
    }

    if (!table || !stmt) {
        result->result = EXEC_NO_TABLE;
        return result;
    }

    size_t match_count = 0;
    size_t row_count = table_row_count(table);

    /* 모든 행을 스캔하면서 WHERE 조건 확인 */
    for (size_t i = 0; i < row_count; i++) {
        void *row = table_get_row(table, i);
        if (row && matches_where(table, row, stmt)) {
            match_count++;
        }
    }

    result->result = EXEC_OK;
    result->result_rows = match_count;
    result->affected_rows = 0;

    return result;
}

/* INSERT 실행 */
QueryResult* execute_insert(Table *table, SQLStatement *stmt) {
    QueryResult *result = (QueryResult *)mm_alloc(sizeof(QueryResult));
    if (!result) return NULL;

    result->error_msg = string_new_with_capacity(128);
    if (!result->error_msg) {
        mm_free(result);
        return NULL;
    }

    if (!table || !stmt) {
        result->result = EXEC_NO_TABLE;
        return result;
    }

    size_t col_count = sql_column_count(stmt);
    size_t val_count = sql_value_count(stmt);

    if (col_count != val_count) {
        result->result = EXEC_INVALID_COL;
        string_append(result->error_msg, "Column count mismatch");
        return result;
    }

    /* 테이블의 컬럼 수와 비교 */
    if (col_count != table_column_count(table)) {
        result->result = EXEC_INVALID_COL;
        string_append(result->error_msg, "Table column count mismatch");
        return result;
    }

    /* 행 데이터 메모리 할당 */
    void *row = mm_alloc(table->row_size);
    if (!row) {
        result->result = EXEC_ERROR;
        return result;
    }

    /* 각 컬럼별로 값을 행 데이터에 복사 */
    for (size_t i = 0; i < col_count; i++) {
        const char *val_str = sql_get_value(stmt, i);
        if (!val_str) {
            mm_free(row);
            result->result = EXEC_ERROR;
            return result;
        }

        Column *col = table_get_column(table, i);
        if (!col) {
            mm_free(row);
            result->result = EXEC_INVALID_COL;
            return result;
        }

        unsigned char *row_bytes = (unsigned char *)row;

        /* 값을 컬럼 타입에 따라 변환 */
        switch (col->type) {
            case COLUMN_TYPE_INT: {
                int val = (int)parse_int(val_str);
                my_memcpy(row_bytes + col->offset, &val, sizeof(int));
                break;
            }
            case COLUMN_TYPE_TEXT: {
                size_t len = my_strlen(val_str);
                if (len >= 256) len = 255;
                my_memcpy(row_bytes + col->offset, val_str, len);
                row_bytes[col->offset + len] = '\0';
                break;
            }
            case COLUMN_TYPE_REAL: {
                double val = parse_double(val_str);
                my_memcpy(row_bytes + col->offset, &val, sizeof(double));
                break;
            }
        }
    }

    /* 테이블에 행 삽입 */
    if (table_insert_row(table, row) < 0) {
        mm_free(row);
        result->result = EXEC_ERROR;
        return result;
    }

    mm_free(row);
    result->result = EXEC_OK;
    result->affected_rows = 1;
    result->result_rows = 0;

    return result;
}

/* UPDATE 실행 */
QueryResult* execute_update(Table *table, SQLStatement *stmt) {
    QueryResult *result = (QueryResult *)mm_alloc(sizeof(QueryResult));
    if (!result) return NULL;

    result->error_msg = string_new_with_capacity(128);
    if (!result->error_msg) {
        mm_free(result);
        return NULL;
    }

    if (!table || !stmt) {
        result->result = EXEC_NO_TABLE;
        return result;
    }

    size_t row_count = table_row_count(table);
    size_t updated = 0;

    /* 모든 행을 스캔하면서 WHERE 조건 확인 */
    for (size_t i = 0; i < row_count; i++) {
        void *row = table_get_row(table, i);
        if (row && matches_where(table, row, stmt)) {
            /* 이 행을 UPDATE */
            /* 단순화: 첫 번째 값을 첫 번째 컬럼에 적용 */
            const char *val_str = sql_get_value(stmt, 0);
            if (val_str) {
                Column *col = table_get_column(table, 0);
                if (col) {
                    unsigned char *row_bytes = (unsigned char *)row;

                    switch (col->type) {
                        case COLUMN_TYPE_INT: {
                            int val = (int)parse_int(val_str);
                            my_memcpy(row_bytes + col->offset, &val, sizeof(int));
                            break;
                        }
                        case COLUMN_TYPE_TEXT: {
                            size_t len = my_strlen(val_str);
                            if (len >= 256) len = 255;
                            my_memcpy(row_bytes + col->offset, val_str, len);
                            row_bytes[col->offset + len] = '\0';
                            break;
                        }
                        case COLUMN_TYPE_REAL: {
                            double val = parse_double(val_str);
                            my_memcpy(row_bytes + col->offset, &val, sizeof(double));
                            break;
                        }
                    }
                    updated++;
                }
            }
        }
    }

    result->result = EXEC_OK;
    result->affected_rows = updated;
    result->result_rows = 0;

    return result;
}

/* DELETE 실행 */
QueryResult* execute_delete(Table *table, SQLStatement *stmt) {
    QueryResult *result = (QueryResult *)mm_alloc(sizeof(QueryResult));
    if (!result) return NULL;

    result->error_msg = string_new_with_capacity(128);
    if (!result->error_msg) {
        mm_free(result);
        return NULL;
    }

    if (!table || !stmt) {
        result->result = EXEC_NO_TABLE;
        return result;
    }

    size_t deleted = 0;
    size_t row_count = table_row_count(table);

    /* 뒤에서 앞으로 스캔 (delete 시 인덱스 변동 때문) */
    for (int i = (int)row_count - 1; i >= 0; i--) {
        void *row = table_get_row(table, i);
        if (row && matches_where(table, row, stmt)) {
            if (table_delete_row(table, i) == 0) {
                deleted++;
            }
        }
    }

    result->result = EXEC_OK;
    result->affected_rows = deleted;
    result->result_rows = 0;

    return result;
}

/* CREATE 실행 */
QueryResult* execute_create(SQLStatement *stmt) {
    QueryResult *result = (QueryResult *)mm_alloc(sizeof(QueryResult));
    if (!result) return NULL;

    result->error_msg = string_new_with_capacity(128);
    if (!result->error_msg) {
        mm_free(result);
        return NULL;
    }

    if (!stmt) {
        result->result = EXEC_ERROR;
        return result;
    }

    const char *table_name = sql_get_table(stmt);
    if (!table_name) {
        result->result = EXEC_ERROR;
        return result;
    }

    /* CREATE는 별도로 처리 필요 (테이블 레지스트리 필요) */
    result->result = EXEC_OK;
    result->affected_rows = 0;
    result->result_rows = 0;

    return result;
}

/* 메인 실행 함수 */
QueryResult* execute(Table *table, SQLStatement *stmt) {
    if (!stmt) return NULL;

    SQLType type = sql_get_type(stmt);

    switch (type) {
        case SQL_SELECT:
            return execute_select(table, stmt);
        case SQL_INSERT:
            return execute_insert(table, stmt);
        case SQL_UPDATE:
            return execute_update(table, stmt);
        case SQL_DELETE:
            return execute_delete(table, stmt);
        case SQL_CREATE:
            return execute_create(stmt);
        default: {
            QueryResult *result = (QueryResult *)mm_alloc(sizeof(QueryResult));
            if (result) {
                result->result = EXEC_ERROR;
                result->error_msg = string_new_with_capacity(128);
                if (result->error_msg) {
                    string_append(result->error_msg, "Unknown SQL type");
                }
            }
            return result;
        }
    }
}

/* 메모리 해제 */
void query_result_free(QueryResult *result) {
    if (!result) return;

    if (result->error_msg) {
        string_free(result->error_msg);
    }

    mm_free(result);
}
