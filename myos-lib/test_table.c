#include <stdio.h>
#include "src/mm.h"
#include "src/table.h"

static int passed = 0, failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); passed++; } \
} while(0)

void test_table_creation(void) {
    TEST("E-1.1: Table creation");

    Table *table = table_new("users");
    ASSERT(table != NULL, "Table created");
    ASSERT(table_row_count(table) == 0, "Initial row count is 0");
    ASSERT(table_column_count(table) == 0, "Initial column count is 0");

    table_free(table);
}

void test_table_add_columns(void) {
    TEST("E-1.2: Add columns");

    Table *table = table_new("users");
    ASSERT(table != NULL, "Table created");

    /* INT 컬럼 추가 */
    int ret1 = table_add_column(table, "id", COLUMN_TYPE_INT);
    ASSERT(ret1 == 0, "INT column added");
    ASSERT(table_column_count(table) == 1, "Column count is 1");

    /* TEXT 컬럼 추가 */
    int ret2 = table_add_column(table, "name", COLUMN_TYPE_TEXT);
    ASSERT(ret2 == 0, "TEXT column added");
    ASSERT(table_column_count(table) == 2, "Column count is 2");

    /* REAL 컬럼 추가 */
    int ret3 = table_add_column(table, "score", COLUMN_TYPE_REAL);
    ASSERT(ret3 == 0, "REAL column added");
    ASSERT(table_column_count(table) == 3, "Column count is 3");

    /* 컬럼 정보 확인 */
    Column *col = table_get_column(table, 0);
    ASSERT(col != NULL && col->type == COLUMN_TYPE_INT, "First column is INT");

    table_free(table);
}

void test_table_insert_rows(void) {
    TEST("E-1.3: Insert rows");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* 행 데이터 구성 (INT + TEXT) */
    uint8_t row_data[sizeof(int) + 256];
    *(int *)row_data = 1;  /* id = 1 */

    /* 첫 번째 행 삽입 */
    int ret1 = table_insert_row(table, row_data);
    ASSERT(ret1 == 0, "First row inserted");
    ASSERT(table_row_count(table) == 1, "Row count is 1");

    /* 두 번째 행 삽입 */
    *(int *)row_data = 2;  /* id = 2 */
    int ret2 = table_insert_row(table, row_data);
    ASSERT(ret2 == 0, "Second row inserted");
    ASSERT(table_row_count(table) == 2, "Row count is 2");

    /* 행 데이터 조회 */
    void *retrieved = table_get_row(table, 0);
    ASSERT(retrieved != NULL, "Row retrieved");
    ASSERT(*(int *)retrieved == 1, "Row data matches");

    table_free(table);
}

void test_table_delete_rows(void) {
    TEST("E-1.4: Delete rows");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);

    /* 3개 행 삽입 */
    uint8_t row_data[sizeof(int) + 256];
    for (int i = 1; i <= 3; i++) {
        *(int *)row_data = i;
        table_insert_row(table, row_data);
    }

    ASSERT(table_row_count(table) == 3, "3 rows inserted");

    /* 첫 번째 행 삭제 */
    int ret = table_delete_row(table, 0);
    ASSERT(ret == 0, "Row deleted");
    ASSERT(table_row_count(table) == 2, "Row count is 2");

    /* 두 번째 행 확인 (마지막 행과 교환됨) */
    void *remaining = table_get_row(table, 0);
    ASSERT(*(int *)remaining == 3, "Last row moved to index 0");

    table_free(table);
}

void test_table_find_column(void) {
    TEST("E-1.5: Find column by name");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);
    table_add_column(table, "score", COLUMN_TYPE_REAL);

    /* 컬럼 검색 */
    int idx1 = table_find_column(table, "id");
    ASSERT(idx1 == 0, "Found 'id' at index 0");

    int idx2 = table_find_column(table, "name");
    ASSERT(idx2 == 1, "Found 'name' at index 1");

    int idx3 = table_find_column(table, "score");
    ASSERT(idx3 == 2, "Found 'score' at index 2");

    /* 존재하지 않는 컬럼 */
    int idx_not_found = table_find_column(table, "age");
    ASSERT(idx_not_found == -1, "Column 'age' not found");

    table_free(table);
}

void test_table_info(void) {
    TEST("E-1.6: Get table info");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* 2개 행 추가 */
    uint8_t row_data[sizeof(int) + 256];
    for (int i = 1; i <= 2; i++) {
        *(int *)row_data = i;
        table_insert_row(table, row_data);
    }

    /* 테이블 정보 조회 */
    char *table_name = NULL;
    size_t col_count = 0, row_count = 0;
    table_get_info(table, &table_name, &col_count, &row_count);

    ASSERT(table_name != NULL, "Table name retrieved");
    ASSERT(col_count == 2, "Column count is 2");
    ASSERT(row_count == 2, "Row count is 2");

    table_free(table);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase E-1: Table Management - 6 Tests\n");
    printf("═════════════════════════════════════════════════════\n");

    mm_init(8 * 1024 * 1024);

    test_table_creation();
    test_table_add_columns();
    test_table_insert_rows();
    test_table_delete_rows();
    test_table_find_column();
    test_table_info();

    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");

    return (failed > 0) ? 1 : 0;
}
