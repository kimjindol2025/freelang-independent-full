#include "sql_parser.h"
#include "mm.h"
#include "string.h"

static void* my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static size_t my_strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static int my_strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return 1;
        if (!s1[i]) break;
    }
    return 0;
}

static char to_upper(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

/* 공백 건너뛰기 */
static const char* skip_spaces(const char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    return s;
}

/* 토큰 추출 (쉼표, 공백, 괄호로 분리) */
static String* extract_token(const char **s) {
    *s = skip_spaces(*s);
    if (!**s) return NULL;

    String *token = string_new_with_capacity(64);
    if (!token) return NULL;

    while (**s && **s != ',' && **s != ' ' && **s != '\t' && **s != '\n'
           && **s != '(' && **s != ')') {
        char c = **s;
        string_append_char(token, c);
        (*s)++;
    }

    return token;
}

/* WHERE 절 추출 */
static String* extract_where(const char *s) {
    while (*s) {
        if (my_strncmp(s, "WHERE", 5) == 0) {
            s += 5;
            s = skip_spaces(s);
            
            String *where = string_new_with_capacity(256);
            if (!where) return NULL;

            while (*s && *s != ';') {
                string_append_char(where, *s);
                s++;
            }

            return where;
        }
        s++;
    }

    return NULL;
}

/* SQL 타입 식별 */
static SQLType identify_type(const char *s) {
    s = skip_spaces(s);

    if (my_strncmp(s, "SELECT", 6) == 0) return SQL_SELECT;
    if (my_strncmp(s, "INSERT", 6) == 0) return SQL_INSERT;
    if (my_strncmp(s, "UPDATE", 6) == 0) return SQL_UPDATE;
    if (my_strncmp(s, "DELETE", 6) == 0) return SQL_DELETE;
    if (my_strncmp(s, "CREATE", 6) == 0) return SQL_CREATE;

    return SQL_UNKNOWN;
}

SQLStatement* sql_parse(const char *sql_query) {
    if (!sql_query) return NULL;

    SQLStatement *stmt = (SQLStatement *)mm_alloc(sizeof(SQLStatement));
    if (!stmt) return NULL;

    stmt->type = identify_type(sql_query);
    stmt->table_name = NULL;
    stmt->columns = vector_new(sizeof(String*));
    stmt->values = vector_new(sizeof(String*));
    stmt->where_clause = NULL;
    stmt->error_msg = string_new_with_capacity(128);

    if (!stmt->columns || !stmt->values) {
        sql_statement_free(stmt);
        return NULL;
    }

    const char *pos = sql_query;

    switch (stmt->type) {
        case SQL_SELECT: {
            /* SELECT col1, col2 FROM table WHERE ... */
            pos = skip_spaces(pos + 6);

            /* 컬럼 목록 추출 */
            while (*pos && my_strncmp(pos, "FROM", 4) != 0) {
                String *col = extract_token(&pos);
                if (col && string_length(col) > 0) {
                    vector_push(stmt->columns, &col);
                } else if (col) {
                    string_free(col);
                }
                pos = skip_spaces(pos);
                if (*pos == ',') pos++;
            }

            /* FROM 테이블명 추출 */
            if (my_strncmp(pos, "FROM", 4) == 0) {
                pos = skip_spaces(pos + 4);
                stmt->table_name = extract_token(&pos);
            }

            /* WHERE 절 추출 */
            stmt->where_clause = extract_where(sql_query);

            break;
        }

        case SQL_INSERT: {
            /* INSERT INTO table (col1, col2) VALUES (val1, val2) */
            pos = skip_spaces(pos + 6);
            if (my_strncmp(pos, "INTO", 4) == 0) {
                pos = skip_spaces(pos + 4);
                stmt->table_name = extract_token(&pos);
            }

            /* 컬럼 목록 (선택사항) */
            pos = skip_spaces(pos);
            if (*pos == '(') {
                pos++;
                while (*pos && *pos != ')') {
                    String *col = extract_token(&pos);
                    if (col && string_length(col) > 0) {
                        vector_push(stmt->columns, &col);
                    } else if (col) {
                        string_free(col);
                    }
                    pos = skip_spaces(pos);
                    if (*pos == ',') pos++;
                }
                pos++;  /* ) 건너뛰기 */
            }

            /* VALUES 추출 */
            pos = skip_spaces(pos);
            if (my_strncmp(pos, "VALUES", 6) == 0) {
                pos = skip_spaces(pos + 6);
                if (*pos == '(') {
                    pos++;
                    while (*pos && *pos != ')') {
                        String *val = extract_token(&pos);
                        if (val && string_length(val) > 0) {
                            vector_push(stmt->values, &val);
                        } else if (val) {
                            string_free(val);
                        }
                        pos = skip_spaces(pos);
                        if (*pos == ',') pos++;
                    }
                }
            }

            break;
        }

        case SQL_UPDATE: {
            /* UPDATE table SET col1=val1 WHERE ... */
            pos = skip_spaces(pos + 6);
            stmt->table_name = extract_token(&pos);

            /* SET 절은 간단하게 VALUE로만 저장 */
            pos = skip_spaces(pos);
            if (my_strncmp(pos, "SET", 3) == 0) {
                pos = skip_spaces(pos + 3);
                while (*pos && my_strncmp(pos, "WHERE", 5) != 0 && *pos != ';') {
                    String *val = extract_token(&pos);
                    if (val && string_length(val) > 0) {
                        vector_push(stmt->values, &val);
                    } else if (val) {
                        string_free(val);
                    }
                    pos = skip_spaces(pos);
                    if (*pos == ',') pos++;
                }
            }

            stmt->where_clause = extract_where(sql_query);

            break;
        }

        case SQL_DELETE: {
            /* DELETE FROM table WHERE ... */
            pos = skip_spaces(pos + 6);
            if (my_strncmp(pos, "FROM", 4) == 0) {
                pos = skip_spaces(pos + 4);
                stmt->table_name = extract_token(&pos);
            }

            stmt->where_clause = extract_where(sql_query);

            break;
        }

        case SQL_CREATE: {
            /* CREATE TABLE name (col1 INT, ...) */
            pos = skip_spaces(pos + 6);
            if (my_strncmp(pos, "TABLE", 5) == 0) {
                pos = skip_spaces(pos + 5);
                stmt->table_name = extract_token(&pos);
            }

            /* 컬럼 정의 (괄호 안) */
            pos = skip_spaces(pos);
            if (*pos == '(') {
                pos++;
                while (*pos && *pos != ')') {
                    String *col = extract_token(&pos);
                    if (col && string_length(col) > 0) {
                        vector_push(stmt->columns, &col);
                    } else if (col) {
                        string_free(col);
                    }
                    pos = skip_spaces(pos);
                    if (*pos == ',') pos++;
                }
            }

            break;
        }

        default:
            string_append(stmt->error_msg, "Unknown SQL command");
            break;
    }

    return stmt;
}

void sql_statement_free(SQLStatement *stmt) {
    if (!stmt) return;

    if (stmt->table_name) string_free(stmt->table_name);

    if (stmt->columns) {
        for (size_t i = 0; i < vector_size(stmt->columns); i++) {
            String *col = *(String **)vector_at(stmt->columns, i);
            if (col) string_free(col);
        }
        vector_free(stmt->columns);
    }

    if (stmt->values) {
        for (size_t i = 0; i < vector_size(stmt->values); i++) {
            String *val = *(String **)vector_at(stmt->values, i);
            if (val) string_free(val);
        }
        vector_free(stmt->values);
    }

    if (stmt->where_clause) string_free(stmt->where_clause);
    if (stmt->error_msg) string_free(stmt->error_msg);

    mm_free(stmt);
}

SQLType sql_get_type(SQLStatement *stmt) {
    return stmt ? stmt->type : SQL_UNKNOWN;
}

const char* sql_get_table(SQLStatement *stmt) {
    if (!stmt || !stmt->table_name) return NULL;
    return string_c_str(stmt->table_name);
}

size_t sql_column_count(SQLStatement *stmt) {
    return stmt ? vector_size(stmt->columns) : 0;
}

const char* sql_get_column(SQLStatement *stmt, size_t index) {
    if (!stmt || index >= vector_size(stmt->columns)) return NULL;

    String *col = *(String **)vector_at(stmt->columns, index);
    return col ? string_c_str(col) : NULL;
}

size_t sql_value_count(SQLStatement *stmt) {
    return stmt ? vector_size(stmt->values) : 0;
}

const char* sql_get_value(SQLStatement *stmt, size_t index) {
    if (!stmt || index >= vector_size(stmt->values)) return NULL;

    String *val = *(String **)vector_at(stmt->values, index);
    return val ? string_c_str(val) : NULL;
}

const char* sql_get_where(SQLStatement *stmt) {
    if (!stmt || !stmt->where_clause) return NULL;
    return string_c_str(stmt->where_clause);
}

const char* sql_get_error(SQLStatement *stmt) {
    if (!stmt || !stmt->error_msg) return NULL;
    return string_c_str(stmt->error_msg);
}
