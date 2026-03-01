#include "sql_manager.h"
#include "mm.h"

/* 테이블 이름 해시 함수 */
static uint32_t hash_table_name(const void *key, size_t key_size) {
    const char *name = *(const char *const *)key;
    uint32_t hash = 5381;
    int c;
    while ((c = *name++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
    (void)key_size;  /* 사용하지 않음 */
}

/* 테이블 이름 비교 함수 */
static int compare_table_name(const void *a, const void *b) {
    const char *name1 = *(const char *const *)a;
    const char *name2 = *(const char *const *)b;

    if (!name1 || !name2) return (name1 == name2) ? 0 : 1;

    while (*name1 && *name2) {
        if (*name1 != *name2) return 1;
        name1++; name2++;
    }
    return (*name1 != *name2) ? 1 : 0;
}

/* SQL 관리자 생성 */
SQLManager* sql_manager_new(void) {
    SQLManager *mgr = (SQLManager *)mm_alloc(sizeof(SQLManager));
    if (!mgr) return NULL;

    /* key: char*, value: Table* */
    mgr->tables = hash_new(sizeof(char*), sizeof(Table*),
                           hash_table_name, compare_table_name);
    if (!mgr->tables) {
        mm_free(mgr);
        return NULL;
    }

    mgr->table_count = 0;

    return mgr;
}

/* 테이블 추가 */
int sql_manager_add_table(SQLManager *mgr, Table *table) {
    if (!mgr || !table || !table->table_name) return -1;

    /* 중복 확인 */
    if (sql_manager_get_table(mgr, table->table_name) != NULL) {
        return -1;  /* 이미 존재 */
    }

    const char *name = table->table_name;
    if (hash_set(mgr->tables, &name, &table) != 0) {
        return -1;
    }

    mgr->table_count++;
    return 0;
}

/* 테이블 조회 */
Table* sql_manager_get_table(SQLManager *mgr, const char *name) {
    if (!mgr || !name) return NULL;

    void *result = hash_get(mgr->tables, &name);
    if (!result) return NULL;

    Table **table_ptr = (Table **)result;
    return *table_ptr;
}

/* 테이블 삭제 */
int sql_manager_drop_table(SQLManager *mgr, const char *name) {
    if (!mgr || !name) return -1;

    Table *table = sql_manager_get_table(mgr, name);
    if (!table) return -1;

    /* 테이블 메모리 해제 */
    table_free(table);

    /* HashMap에서 제거 */
    Table *dummy = NULL;
    if (hash_delete(mgr->tables, &name, &dummy) != 0) {
        return -1;
    }

    mgr->table_count--;
    return 0;
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
        size_t table_count = hash_size(mgr->tables);
        for (size_t i = 0; i < table_count; i++) {
            /* Vector처럼 직접 접근할 수 없으므로, iterator 필요 */
            /* 여기서는 단순히 count만 사용 */
        }

        hash_free(mgr->tables);
    }

    mm_free(mgr);
}
