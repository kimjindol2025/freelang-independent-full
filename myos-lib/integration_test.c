#include <stdio.h>
#include "src/mm.h"
#include "src/table.h"
#include "src/sql_parser.h"
#include "src/sql_executor.h"
#include "src/sql_manager.h"

static int passed = 0, failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); passed++; } \
} while(0)

static int get_elapsed_ms(void) {
    /* 간단한 시뮬레이션 - 실제로는 clock() 사용 */
    return 0;
}

void test_integration_setup(void) {
    TEST("F-1.1: Setup with Manager");

    SQLManager *mgr = sql_manager_new();
    ASSERT(mgr != NULL, "Manager created");
    ASSERT(sql_manager_table_count(mgr) == 0, "Initial count is 0");

    /* 테이블 생성 */
    Table *users = table_new("users");
    ASSERT(users != NULL, "Table created");

    /* 컬럼 추가 */
    table_add_column(users, "id", COLUMN_TYPE_INT);
    table_add_column(users, "name", COLUMN_TYPE_TEXT);
    table_add_column(users, "score", COLUMN_TYPE_REAL);

    /* 관리자에 테이블 추가 */
    int ret = sql_manager_add_table(mgr, users);
    ASSERT(ret == 0, "Table added to manager");
    ASSERT(sql_manager_table_count(mgr) == 1, "Manager has 1 table");

    /* 테이블 조회 */
    Table *retrieved = sql_manager_get_table(mgr, "users");
    ASSERT(retrieved != NULL, "Table retrieved");
    ASSERT(retrieved == users, "Retrieved table is same object");

    sql_manager_free(mgr);
}

void test_integration_bulk_insert(void) {
    TEST("F-1.2: Bulk INSERT 100 rows");

    Table *users = table_new("users");
    table_add_column(users, "id", COLUMN_TYPE_INT);
    table_add_column(users, "name", COLUMN_TYPE_TEXT);

    int inserted = 0;
    for (int i = 1; i <= 100; i++) {
        char query[256];
        sprintf(query, "INSERT INTO users (id, name) VALUES (%d, user%d)", i, i);

        SQLStatement *stmt = sql_parse(query);
        if (stmt) {
            QueryResult *result = execute(users, stmt);
            if (result && result->affected_rows == 1) {
                inserted++;
            }
            if (result) query_result_free(result);
            sql_statement_free(stmt);
        }
    }

    ASSERT(inserted == 100, "All 100 rows inserted");
    ASSERT(table_row_count(users) == 100, "Table has 100 rows");

    table_free(users);
}

void test_integration_select_various(void) {
    TEST("F-1.3: SELECT with various WHERE conditions");

    Table *users = table_new("users");
    table_add_column(users, "id", COLUMN_TYPE_INT);
    table_add_column(users, "name", COLUMN_TYPE_TEXT);

    /* 데이터 준비: 10개 행 */
    for (int i = 1; i <= 10; i++) {
        char query[256];
        sprintf(query, "INSERT INTO users (id, name) VALUES (%d, user%d)", i, i);
        SQLStatement *stmt = sql_parse(query);
        QueryResult *result = execute(users, stmt);
        if (result) query_result_free(result);
        sql_statement_free(stmt);
    }

    /* SELECT all */
    SQLStatement *select_all = sql_parse("SELECT id, name FROM users");
    QueryResult *result_all = execute(users, select_all);
    ASSERT(result_all->result_rows == 10, "SELECT all returns 10 rows");
    query_result_free(result_all);
    sql_statement_free(select_all);

    /* SELECT id = 1 */
    SQLStatement *select_1 = sql_parse("SELECT id, name FROM users WHERE id = 1");
    QueryResult *result_1 = execute(users, select_1);
    ASSERT(result_1->result_rows == 1, "SELECT id=1 returns 1 row");
    query_result_free(result_1);
    sql_statement_free(select_1);

    /* SELECT id = 5 */
    SQLStatement *select_5 = sql_parse("SELECT id, name FROM users WHERE id = 5");
    QueryResult *result_5 = execute(users, select_5);
    ASSERT(result_5->result_rows == 1, "SELECT id=5 returns 1 row");
    query_result_free(result_5);
    sql_statement_free(select_5);

    /* SELECT non-existent */
    SQLStatement *select_999 = sql_parse("SELECT id, name FROM users WHERE id = 999");
    QueryResult *result_999 = execute(users, select_999);
    ASSERT(result_999->result_rows == 0, "SELECT id=999 returns 0 rows");
    query_result_free(result_999);
    sql_statement_free(select_999);

    table_free(users);
}

void test_integration_update_bulk(void) {
    TEST("F-1.4: Bulk UPDATE");

    Table *users = table_new("users");
    table_add_column(users, "id", COLUMN_TYPE_INT);
    table_add_column(users, "name", COLUMN_TYPE_TEXT);

    /* 데이터 준비: 20개 행 */
    for (int i = 1; i <= 20; i++) {
        char query[256];
        sprintf(query, "INSERT INTO users (id, name) VALUES (%d, user%d)", i, i);
        SQLStatement *stmt = sql_parse(query);
        QueryResult *result = execute(users, stmt);
        if (result) query_result_free(result);
        sql_statement_free(stmt);
    }

    /* UPDATE WHERE id = 10 */
    SQLStatement *update = sql_parse("UPDATE users SET name = updated WHERE id = 10");
    QueryResult *result = execute(users, update);
    ASSERT(result->affected_rows == 1, "UPDATE affects 1 row");
    query_result_free(result);
    sql_statement_free(update);

    table_free(users);
}

void test_integration_delete_bulk(void) {
    TEST("F-1.5: Bulk DELETE");

    Table *users = table_new("users");
    table_add_column(users, "id", COLUMN_TYPE_INT);
    table_add_column(users, "name", COLUMN_TYPE_TEXT);

    /* 데이터 준비: 30개 행 */
    for (int i = 1; i <= 30; i++) {
        char query[256];
        sprintf(query, "INSERT INTO users (id, name) VALUES (%d, user%d)", i, i);
        SQLStatement *stmt = sql_parse(query);
        QueryResult *result = execute(users, stmt);
        if (result) query_result_free(result);
        sql_statement_free(stmt);
    }

    ASSERT(table_row_count(users) == 30, "Initial: 30 rows");

    /* DELETE WHERE id >= 20 */
    /* 참고: 단순 "=" 조건만 지원하므로, id=20 하나만 삭제 */
    SQLStatement *delete = sql_parse("DELETE FROM users WHERE id = 20");
    QueryResult *result = execute(users, delete);
    ASSERT(result->affected_rows == 1, "DELETE removes 1 row");
    query_result_free(result);
    sql_statement_free(delete);

    ASSERT(table_row_count(users) == 29, "After DELETE: 29 rows");

    table_free(users);
}

void test_integration_mixed_operations(void) {
    TEST("F-1.6: Mixed operations");

    Table *products = table_new("products");
    table_add_column(products, "id", COLUMN_TYPE_INT);
    table_add_column(products, "name", COLUMN_TYPE_TEXT);

    /* INSERT 5개 */
    for (int i = 1; i <= 5; i++) {
        char query[256];
        sprintf(query, "INSERT INTO products (id, name) VALUES (%d, prod%d)", i, i);
        SQLStatement *stmt = sql_parse(query);
        QueryResult *result = execute(products, stmt);
        if (result) query_result_free(result);
        sql_statement_free(stmt);
    }
    ASSERT(table_row_count(products) == 5, "After INSERT: 5 rows");

    /* SELECT all */
    SQLStatement *select = sql_parse("SELECT id, name FROM products");
    QueryResult *result_select = execute(products, select);
    ASSERT(result_select->result_rows == 5, "SELECT returns 5 rows");
    query_result_free(result_select);
    sql_statement_free(select);

    /* UPDATE */
    SQLStatement *update = sql_parse("UPDATE products SET name = updated WHERE id = 1");
    QueryResult *result_update = execute(products, update);
    ASSERT(result_update->affected_rows == 1, "UPDATE affects 1 row");
    query_result_free(result_update);
    sql_statement_free(update);

    /* DELETE */
    SQLStatement *delete = sql_parse("DELETE FROM products WHERE id = 3");
    QueryResult *result_delete = execute(products, delete);
    ASSERT(result_delete->affected_rows == 1, "DELETE removes 1 row");
    query_result_free(result_delete);
    sql_statement_free(delete);

    ASSERT(table_row_count(products) == 4, "Final: 4 rows");

    table_free(products);
}

void test_integration_error_handling(void) {
    TEST("F-1.7: Error handling");

    SQLManager *mgr = sql_manager_new();
    Table *users = table_new("users");
    table_add_column(users, "id", COLUMN_TYPE_INT);

    /* 테이블 추가 */
    sql_manager_add_table(mgr, users);

    /* 중복 추가 시도 */
    int ret = sql_manager_add_table(mgr, users);
    ASSERT(ret == -1, "Duplicate add fails");

    /* 존재하지 않는 테이블 조회 */
    Table *nonexistent = sql_manager_get_table(mgr, "nonexistent");
    ASSERT(nonexistent == NULL, "Non-existent table returns NULL");

    /* 존재하지 않는 테이블 삭제 */
    int drop_ret = sql_manager_drop_table(mgr, "nonexistent");
    ASSERT(drop_ret == -1, "Drop non-existent table fails");

    sql_manager_free(mgr);
}

void test_integration_multiple_tables(void) {
    TEST("F-1.8: Multiple tables in manager");

    SQLManager *mgr = sql_manager_new();

    /* 테이블 1: users */
    Table *users = table_new("users");
    table_add_column(users, "id", COLUMN_TYPE_INT);
    table_add_column(users, "name", COLUMN_TYPE_TEXT);
    sql_manager_add_table(mgr, users);

    /* 테이블 2: products */
    Table *products = table_new("products");
    table_add_column(products, "id", COLUMN_TYPE_INT);
    table_add_column(products, "price", COLUMN_TYPE_REAL);
    sql_manager_add_table(mgr, products);

    ASSERT(sql_manager_table_count(mgr) == 2, "Manager has 2 tables");

    /* 각 테이블 조회 */
    Table *u = sql_manager_get_table(mgr, "users");
    Table *p = sql_manager_get_table(mgr, "products");
    ASSERT(u != NULL, "users table found");
    ASSERT(p != NULL, "products table found");
    ASSERT(table_column_count(u) == 2, "users has 2 columns");
    ASSERT(table_column_count(p) == 2, "products has 2 columns");

    sql_manager_free(mgr);
}

void test_integration_concurrent_queries(void) {
    TEST("F-1.9: Concurrent-like queries");

    Table *table = table_new("data");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* 여러 작업을 순차적으로 수행 */
    int success = 0;

    for (int i = 0; i < 5; i++) {
        /* INSERT */
        char insert_query[256];
        sprintf(insert_query, "INSERT INTO data (id, name) VALUES (%d, item%d)", i + 1, i + 1);
        SQLStatement *insert = sql_parse(insert_query);
        QueryResult *r1 = execute(table, insert);
        if (r1 && r1->affected_rows == 1) success++;
        if (r1) query_result_free(r1);
        sql_statement_free(insert);

        /* SELECT */
        char select_query[256];
        sprintf(select_query, "SELECT id, name FROM data WHERE id = %d", i + 1);
        SQLStatement *select = sql_parse(select_query);
        QueryResult *r2 = execute(table, select);
        if (r2 && r2->result_rows == 1) success++;
        if (r2) query_result_free(r2);
        sql_statement_free(select);
    }

    ASSERT(success == 10, "All 10 operations succeed");
    ASSERT(table_row_count(table) == 5, "Table has 5 rows");

    table_free(table);
}

void test_integration_performance(void) {
    TEST("F-1.10: Performance baseline");

    Table *table = table_new("perf_test");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* INSERT 50개 */
    int inserted = 0;
    for (int i = 1; i <= 50; i++) {
        char query[256];
        sprintf(query, "INSERT INTO perf_test (id, name) VALUES (%d, item%d)", i, i);
        SQLStatement *stmt = sql_parse(query);
        QueryResult *result = execute(table, stmt);
        if (result && result->affected_rows == 1) inserted++;
        if (result) query_result_free(result);
        sql_statement_free(stmt);
    }

    ASSERT(inserted == 50, "Inserted 50 rows");

    /* SELECT 조건 조회 */
    SQLStatement *select = sql_parse("SELECT id FROM perf_test WHERE id = 25");
    QueryResult *result = execute(table, select);
    ASSERT(result && result->result_rows == 1, "SELECT WHERE returns 1 row");
    if (result) query_result_free(result);
    sql_statement_free(select);

    table_free(table);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase F-1: SQL Integration - 10 Tests\n");
    printf("═════════════════════════════════════════════════════\n");

    mm_init(16 * 1024 * 1024);  /* 16MB 할당 */

    test_integration_setup();
    test_integration_bulk_insert();
    test_integration_select_various();
    test_integration_update_bulk();
    test_integration_delete_bulk();
    test_integration_mixed_operations();
    test_integration_error_handling();
    test_integration_multiple_tables();
    test_integration_concurrent_queries();
    test_integration_performance();

    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");

    return (failed > 0) ? 1 : 0;
}
