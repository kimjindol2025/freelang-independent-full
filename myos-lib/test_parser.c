#include <stdio.h>
#include "src/mm.h"
#include "src/sql_parser.h"

static int passed = 0, failed = 0;

#define TEST(name) printf("\n[TEST] %s\n", name)
#define ASSERT(cond, msg) do { \
    if (!(cond)) { printf("  ❌ FAILED: %s\n", msg); failed++; } \
    else { printf("  ✅ PASSED: %s\n", msg); passed++; } \
} while(0)

void test_parse_select(void) {
    TEST("E-2.1: Parse SELECT");

    SQLStatement *stmt = sql_parse("SELECT id, name FROM users WHERE id = 1");
    ASSERT(stmt != NULL, "Statement parsed");
    ASSERT(sql_get_type(stmt) == SQL_SELECT, "Type is SELECT");
    ASSERT(sql_get_table(stmt) != NULL && 
           ((const char*)sql_get_table(stmt)[0] == 'u' || 
            (const char*)sql_get_table(stmt)[0] == 'U'), "Table is users");
    ASSERT(sql_column_count(stmt) >= 2, "Column count >= 2");
    ASSERT(sql_get_where(stmt) != NULL, "WHERE clause present");

    sql_statement_free(stmt);
}

void test_parse_insert(void) {
    TEST("E-2.2: Parse INSERT");

    SQLStatement *stmt = sql_parse("INSERT INTO users (id, name) VALUES (1, alice)");
    ASSERT(stmt != NULL, "Statement parsed");
    ASSERT(sql_get_type(stmt) == SQL_INSERT, "Type is INSERT");
    ASSERT(sql_get_table(stmt) != NULL, "Table name present");
    ASSERT(sql_column_count(stmt) >= 2, "Columns parsed");
    ASSERT(sql_value_count(stmt) >= 2, "Values parsed");

    sql_statement_free(stmt);
}

void test_parse_update(void) {
    TEST("E-2.3: Parse UPDATE");

    SQLStatement *stmt = sql_parse("UPDATE users SET name = alice WHERE id = 1");
    ASSERT(stmt != NULL, "Statement parsed");
    ASSERT(sql_get_type(stmt) == SQL_UPDATE, "Type is UPDATE");
    ASSERT(sql_get_table(stmt) != NULL, "Table name present");
    ASSERT(sql_get_where(stmt) != NULL, "WHERE clause present");

    sql_statement_free(stmt);
}

void test_parse_delete(void) {
    TEST("E-2.4: Parse DELETE");

    SQLStatement *stmt = sql_parse("DELETE FROM users WHERE id = 1");
    ASSERT(stmt != NULL, "Statement parsed");
    ASSERT(sql_get_type(stmt) == SQL_DELETE, "Type is DELETE");
    ASSERT(sql_get_table(stmt) != NULL, "Table name present");
    ASSERT(sql_get_where(stmt) != NULL, "WHERE clause present");

    sql_statement_free(stmt);
}

void test_parse_error(void) {
    TEST("E-2.5: Error handling");

    /* 빈 쿼리 */
    SQLStatement *stmt1 = sql_parse("");
    ASSERT(stmt1 == NULL || sql_get_type(stmt1) == SQL_UNKNOWN, "Empty query returns unknown");
    if (stmt1) sql_statement_free(stmt1);

    /* NULL 입력 */
    SQLStatement *stmt2 = sql_parse(NULL);
    ASSERT(stmt2 == NULL, "NULL input returns NULL");

    /* 잘못된 쿼리 */
    SQLStatement *stmt3 = sql_parse("INVALID QUERY");
    ASSERT(stmt3 != NULL && sql_get_type(stmt3) == SQL_UNKNOWN, "Invalid query returns UNKNOWN");
    if (stmt3) sql_statement_free(stmt3);
}

int main(void) {
    printf("\n═════════════════════════════════════════════════════\n");
    printf("   Phase E-2: SQL Parser - 5 Tests\n");
    printf("═════════════════════════════════════════════════════\n");

    mm_init(8 * 1024 * 1024);

    test_parse_select();
    test_parse_insert();
    test_parse_update();
    test_parse_delete();
    test_parse_error();

    printf("\n═════════════════════════════════════════════════════\n");
    printf("✅ Passed: %d | ❌ Failed: %d\n", passed, failed);
    printf("═════════════════════════════════════════════════════\n\n");

    return (failed > 0) ? 1 : 0;
}
