#include "sql_manager.h"
#include "vector.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* 간단한 테이블 엔트리 */
typedef struct {
    char *name;
    Table *table;
} TableEntry;

/* SQL 관리자 생성 */
SQLManager* sql_manager_new(void) {
    fprintf(stderr, "[SQLManager] Creating new SQL manager\n");

    SQLManager *mgr = (SQLManager *)malloc(sizeof(SQLManager));
    if (!mgr) {
        fprintf(stderr, "[SQLManager] malloc failed\n");
        return NULL;
    }

    /* Vector를 사용하여 테이블 저장 */
    mgr->tables = vector_new(sizeof(TableEntry));
    if (!mgr->tables) {
        fprintf(stderr, "[SQLManager] vector_new failed\n");
        free(mgr);
        return NULL;
    }

    mgr->table_count = 0;
    fprintf(stderr, "[SQLManager] SQLManager created successfully\n");

    return mgr;
}

/* 테이블 추가 */
int sql_manager_add_table(SQLManager *mgr, Table *table) {
    if (!mgr || !table || !table->table_name) return -1;

    /* 중복 확인 */
    if (sql_manager_get_table(mgr, table->table_name) != NULL) {
        return -1;  /* 이미 존재 */
    }

    TableEntry entry;
    size_t name_len = strlen(table->table_name);
    entry.name = (char *)malloc(name_len + 1);
    if (!entry.name) return -1;
    strcpy(entry.name, table->table_name);
    entry.table = table;

    if (vector_push(mgr->tables, &entry) != 0) {
        free(entry.name);
        return -1;
    }

    mgr->table_count++;
    return 0;
}

/* 테이블 조회 */
Table* sql_manager_get_table(SQLManager *mgr, const char *name) {
    if (!mgr || !name) return NULL;

    size_t count = vector_size(mgr->tables);
    for (size_t i = 0; i < count; i++) {
        TableEntry *entry = (TableEntry *)vector_at(mgr->tables, i);
        if (entry && entry->name && strcmp(entry->name, name) == 0) {
            return entry->table;
        }
    }
    return NULL;
}

/* 테이블 삭제 */
int sql_manager_drop_table(SQLManager *mgr, const char *name) {
    if (!mgr || !name) return -1;

    size_t count = vector_size(mgr->tables);
    for (size_t i = 0; i < count; i++) {
        TableEntry *entry = (TableEntry *)vector_at(mgr->tables, i);
        if (entry && entry->name && strcmp(entry->name, name) == 0) {
            /* 테이블 메모리 해제 */
            if (entry->table) table_free(entry->table);
            free(entry->name);

            /* Vector에서 제거 */
            TableEntry removed;
            vector_remove(mgr->tables, i, &removed);
            mgr->table_count--;
            return 0;
        }
    }
    return -1;
}

/* 테이블 개수 조회 */
size_t sql_manager_table_count(SQLManager *mgr) {
    return mgr ? mgr->table_count : 0;
}

/* SQL 관리자 해제 */
void sql_manager_free(SQLManager *mgr) {
    if (!mgr) return;

    if (mgr->tables) {
        /* 모든 테이블 삭제 */
        size_t count = vector_size(mgr->tables);
        for (size_t i = 0; i < count; i++) {
            TableEntry *entry = (TableEntry *)vector_at(mgr->tables, i);
            if (entry) {
                if (entry->table) table_free(entry->table);
                if (entry->name) free(entry->name);
            }
        }
        vector_free(mgr->tables);
    }

    free(mgr);
}
