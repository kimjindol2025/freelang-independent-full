#include "table.h"
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

Table* table_new(const char *table_name) {
    if (!table_name) return NULL;

    Table *table = (Table *)mm_alloc(sizeof(Table));
    if (!table) return NULL;

    table->columns = vector_new(sizeof(Column));
    table->rows = vector_new(sizeof(void*));

    if (!table->columns || !table->rows) {
        if (table->columns) vector_free(table->columns);
        if (table->rows) vector_free(table->rows);
        mm_free(table);
        return NULL;
    }

    /* 테이블 이름 복사 */
    size_t name_len = my_strlen(table_name);
    if (name_len >= sizeof(table->table_name) - 1) {
        vector_free(table->columns);
        vector_free(table->rows);
        mm_free(table);
        return NULL;
    }
    my_memcpy(table->table_name, table_name, name_len);
    table->table_name[name_len] = '\0';

    table->row_size = 0;
    table->row_count = 0;

    return table;
}

int table_add_column(Table *table, const char *col_name, uint8_t col_type) {
    if (!table || !col_name) return -1;
    if (col_type < COLUMN_TYPE_INT || col_type > COLUMN_TYPE_REAL) return -1;

    Column col;
    col.type = col_type;
    col.offset = table->row_size;

    /* 컬럼 이름 복사 */
    size_t name_len = my_strlen(col_name);
    if (name_len >= sizeof(col.name) - 1) return -1;
    my_memcpy(col.name, col_name, name_len);
    col.name[name_len] = '\0';

    /* 데이터 타입에 따른 크기 추가 */
    switch (col_type) {
        case COLUMN_TYPE_INT:
            table->row_size += sizeof(int);
            break;
        case COLUMN_TYPE_TEXT:
            table->row_size += 256;  /* 최대 255자 문자열 */
            break;
        case COLUMN_TYPE_REAL:
            table->row_size += sizeof(double);
            break;
    }

    /* 컬럼 추가 */
    return vector_push(table->columns, &col);
}

int table_insert_row(Table *table, const void *data) {
    if (!table || !data) return -1;
    if (table->row_size == 0) return -1;  /* 컬럼이 없음 */

    /* 행 데이터 복사 */
    void *row_copy = mm_alloc(table->row_size);
    if (!row_copy) return -1;

    my_memcpy(row_copy, data, table->row_size);

    /* Vector에 추가 */
    if (vector_push(table->rows, &row_copy) < 0) {
        mm_free(row_copy);
        return -1;
    }

    table->row_count++;
    return 0;
}

void* table_get_row(Table *table, size_t row_index) {
    if (!table || row_index >= table->row_count) return NULL;

    void **row_ptr = (void **)vector_at(table->rows, row_index);
    return row_ptr ? *row_ptr : NULL;
}

int table_delete_row(Table *table, size_t row_index) {
    if (!table || row_index >= table->row_count) return -1;

    /* 행 메모리 해제 */
    void **row_ptr = (void **)vector_at(table->rows, row_index);
    if (row_ptr && *row_ptr) {
        mm_free(*row_ptr);
    }

    /* Vector에서 제거 (마지막과 교환) */
    if (row_index < table->row_count - 1) {
        void *last_row = *(void **)vector_at(table->rows, table->row_count - 1);
        *(void **)vector_at(table->rows, row_index) = last_row;
    }

    /* 마지막 요소 제거 */
    void *dummy = NULL;
    vector_pop(table->rows, &dummy);
    table->row_count--;

    return 0;
}

size_t table_column_count(Table *table) {
    return table ? vector_size(table->columns) : 0;
}

size_t table_row_count(Table *table) {
    return table ? table->row_count : 0;
}

Column* table_get_column(Table *table, size_t col_index) {
    if (!table || col_index >= vector_size(table->columns)) return NULL;
    return (Column *)vector_at(table->columns, col_index);
}

int table_find_column(Table *table, const char *col_name) {
    if (!table || !col_name) return -1;

    size_t col_count = vector_size(table->columns);
    for (size_t i = 0; i < col_count; i++) {
        Column *col = (Column *)vector_at(table->columns, i);
        if (col && my_strcmp(col->name, col_name) == 0) {
            return (int)i;
        }
    }

    return -1;
}

void table_get_info(Table *table, char **out_name, size_t *out_cols, size_t *out_rows) {
    if (!table) return;

    if (out_name) *out_name = table->table_name;
    if (out_cols) *out_cols = vector_size(table->columns);
    if (out_rows) *out_rows = table->row_count;
}

void table_free(Table *table) {
    if (!table) return;

    /* 모든 행 메모리 해제 */
    for (size_t i = 0; i < table->row_count; i++) {
        void *row = table_get_row(table, i);
        if (row) mm_free(row);
    }

    if (table->columns) vector_free(table->columns);
    if (table->rows) vector_free(table->rows);
    mm_free(table);
}
