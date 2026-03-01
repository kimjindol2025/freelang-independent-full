#ifndef SQL_MANAGER_H
#define SQL_MANAGER_H

#include "table.h"
#include "hash.h"

/* SQL 관리자 - 테이블 저장소 */
typedef struct {
    HashMap *tables;  /* 테이블명 -> Table* 매핑 */
    size_t table_count;
} SQLManager;

/* 테이블 관리 함수 */
SQLManager* sql_manager_new(void);
int sql_manager_add_table(SQLManager *mgr, Table *table);
Table* sql_manager_get_table(SQLManager *mgr, const char *name);
int sql_manager_drop_table(SQLManager *mgr, const char *name);
size_t sql_manager_table_count(SQLManager *mgr);
void sql_manager_free(SQLManager *mgr);

#endif
