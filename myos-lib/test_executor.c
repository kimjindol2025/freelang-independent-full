#include <stdio.h>
#include "src/mm.h"
#include "src/table.h"
#include "src/sql_parser.h"
#include "src/sql_executor.h"

static int passed = 0, failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); passed++; } \
} while(0)

void test_create_table(void) {
    TEST("E-3.1: CREATE TABLE");

    /* 테이블 생성 */
    Table *table = table_new("users");
    ASSERT(table != NULL, "Table created");

    /* 컬럼 추가 */
    int ret1 = table_add_column(table, "id", COLUMN_TYPE_INT);
    int ret2 = table_add_column(table, "name", COLUMN_TYPE_TEXT);
    ASSERT(ret1 == 0 && ret2 == 0, "Columns added");
    ASSERT(table_column_count(table) == 2, "Column count is 2");

    table_free(table);
}

void test_insert_rows(void) {
    TEST("E-3.2: INSERT rows");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* INSERT 1 쿼리 */
    SQLStatement *stmt1 = sql_parse("INSERT INTO users (id, name) VALUES (1, alice)");
    ASSERT(stmt1 != NULL, "SQL parsed");

    QueryResult *result1 = execute(table, stmt1);
    ASSERT(result1 != NULL, "Query executed");
    ASSERT(result1->result == EXEC_OK, "Query OK");
    ASSERT(result1->affected_rows == 1, "1 row inserted");

    query_result_free(result1);
    sql_statement_free(stmt1);

    /* INSERT 2 쿼리 */
    SQLStatement *stmt2 = sql_parse("INSERT INTO users (id, name) VALUES (2, bob)");
    QueryResult *result2 = execute(table, stmt2);
    ASSERT(result2->affected_rows == 1, "Second row inserted");
    query_result_free(result2);
    sql_statement_free(stmt2);

    ASSERT(table_row_count(table) == 2, "Total 2 rows");

    table_free(table);
}

void test_select_rows(void) {
    TEST("E-3.3: SELECT with WHERE");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* 데이터 준비 */
    SQLStatement *insert1 = sql_parse("INSERT INTO users (id, name) VALUES (1, alice)");
    QueryResult *r1 = execute(table, insert1);
    query_result_free(r1);
    sql_statement_free(insert1);

    SQLStatement *insert2 = sql_parse("INSERT INTO users (id, name) VALUES (2, bob)");
    QueryResult *r2 = execute(table, insert2);
    query_result_free(r2);
    sql_statement_free(insert2);

    /* SELECT all */
    SQLStatement *select_all = sql_parse("SELECT id, name FROM users");
    QueryResult *result_all = execute(table, select_all);
    ASSERT(result_all->result == EXEC_OK, "SELECT all executed");
    ASSERT(result_all->result_rows == 2, "SELECT all returns 2 rows");
    query_result_free(result_all);
    sql_statement_free(select_all);

    /* SELECT with WHERE */
    SQLStatement *select_where = sql_parse("SELECT id, name FROM users WHERE id = 1");
    QueryResult *result_where = execute(table, select_where);
    ASSERT(result_where->result == EXEC_OK, "SELECT WHERE executed");
    ASSERT(result_where->result_rows == 1, "SELECT WHERE returns 1 row");
    query_result_free(result_where);
    sql_statement_free(select_where);

    table_free(table);
}

void test_update_rows(void) {
    TEST("E-3.4: UPDATE rows");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* 데이터 준비 */
    SQLStatement *insert = sql_parse("INSERT INTO users (id, name) VALUES (1, alice)");
    QueryResult *r = execute(table, insert);
    query_result_free(r);
    sql_statement_free(insert);

    /* UPDATE */
    SQLStatement *update = sql_parse("UPDATE users SET name = charlie WHERE id = 1");
    QueryResult *result = execute(table, update);
    ASSERT(result->result == EXEC_OK, "UPDATE executed");
    ASSERT(result->affected_rows == 1, "1 row updated");
    query_result_free(result);
    sql_statement_free(update);

    table_free(table);
}

void test_delete_rows(void) {
    TEST("E-3.5: DELETE rows");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* 데이터 준비 */
    SQLStatement *insert1 = sql_parse("INSERT INTO users (id, name) VALUES (1, alice)");
    QueryResult *r1 = execute(table, insert1);
    query_result_free(r1);
    sql_statement_free(insert1);

    SQLStatement *insert2 = sql_parse("INSERT INTO users (id, name) VALUES (2, bob)");
    QueryResult *r2 = execute(table, insert2);
    query_result_free(r2);
    sql_statement_free(insert2);

    ASSERT(table_row_count(table) == 2, "Initial: 2 rows");

    /* DELETE */
    SQLStatement *delete_stmt = sql_parse("DELETE FROM users WHERE id = 1");
    QueryResult *result = execute(table, delete_stmt);
    ASSERT(result->result == EXEC_OK, "DELETE executed");
    ASSERT(result->affected_rows == 1, "1 row deleted");
    query_result_free(result);
    sql_statement_free(delete_stmt);

    ASSERT(table_row_count(table) == 1, "After DELETE: 1 row");

    table_free(table);
}

void test_error_handling(void) {
    TEST("E-3.6: Error handling");

    Table *table = table_new("users");
    table_add_column(table, "id", COLUMN_TYPE_INT);
    table_add_column(table, "name", COLUMN_TYPE_TEXT);

    /* Column count mismatch */
    SQLStatement *stmt1 = sql_parse("INSERT INTO users (id, name) VALUES (1)");
    QueryResult *result1 = execute(table, stmt1);
    ASSERT(result1->result == EXEC_INVALID_COL, "Column mismatch detected");
    query_result_free(result1);
    sql_statement_free(stmt1);

    /* NULL statement */
    QueryResult *result2 = execute(table, NULL);
    ASSERT(result2 == NULL, "NULL statement returns NULL");

    /* NULL table */
    SQLStatement *stmt3 = sql_parse("SELECT id FROM users");
    QueryResult *result3 = execute(NULL, stmt3);
    ASSERT(result3->result == EXEC_NO_TABLE, "NULL table error");
    query_result_free(result3);
    sql_statement_free(stmt3);

    table_free(table);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase E-3: SQL Executor - 6 Tests\n");
    printf("═════════════════════════════════════════════════════\n");

    mm_init(8 * 1024 * 1024);

    test_create_table();
    test_insert_rows();
    test_select_rows();
    test_update_rows();
    test_delete_rows();
    test_error_handling();

    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");

    return (failed > 0) ? 1 : 0;
}
