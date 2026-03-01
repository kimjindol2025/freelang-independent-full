#ifndef MYOS_TABLE_H
#define MYOS_TABLE_H

#include <stddef.h>
#include <stdint.h>
#include "vector.h"
#include "string.h"

/* ============================================================================
 * SQL Table Management - 테이블 및 스키마 관리
 * ============================================================================ */

/* 컬럼 데이터 타입 */
#define COLUMN_TYPE_INT     1
#define COLUMN_TYPE_TEXT    2
#define COLUMN_TYPE_REAL    3

/* 컬럼 정의 */
typedef struct {
    char name[32];              /* 컬럼명 */
    uint8_t type;               /* COLUMN_TYPE_* */
    size_t offset;              /* 행 데이터 내 오프셋 */
} Column;

/* 테이블 정의 */
typedef struct {
    char table_name[64];        /* 테이블명 */
    Vector *columns;            /* Column* 배열 */
    Vector *rows;               /* 행 데이터 (void*) */
    size_t row_size;            /* 각 행의 크기 (바이트) */
    size_t row_count;           /* 현재 행 개수 */
} Table;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * 테이블 생성
 * @param table_name: 테이블 이름
 * @return: 생성된 테이블 또는 NULL
 */
Table* table_new(const char *table_name);

/**
 * 테이블에 컬럼 추가
 * @param table: 테이블 객체
 * @param col_name: 컬럼 이름
 * @param col_type: 컬럼 타입 (COLUMN_TYPE_*)
 * @return: 0 (성공), -1 (실패)
 */
int table_add_column(Table *table, const char *col_name, uint8_t col_type);

/**
 * 테이블에 행 추가
 * @param table: 테이블 객체
 * @param data: 행 데이터 (raw bytes)
 * @return: 0 (성공), -1 (실패)
 */
int table_insert_row(Table *table, const void *data);

/**
 * 테이블에서 행 조회
 * @param table: 테이블 객체
 * @param row_index: 행 인덱스
 * @return: 행 데이터 포인터 또는 NULL
 */
void* table_get_row(Table *table, size_t row_index);

/**
 * 테이블에서 행 삭제
 * @param table: 테이블 객체
 * @param row_index: 행 인덱스
 * @return: 0 (성공), -1 (실패)
 */
int table_delete_row(Table *table, size_t row_index);

/**
 * 컬럼 개수 조회
 */
size_t table_column_count(Table *table);

/**
 * 행 개수 조회
 */
size_t table_row_count(Table *table);

/**
 * 컬럼 정보 조회
 * @param table: 테이블 객체
 * @param col_index: 컬럼 인덱스
 * @return: Column 구조체 또는 NULL
 */
Column* table_get_column(Table *table, size_t col_index);

/**
 * 컬럼 이름으로 인덱스 찾기
 * @return: 컬럼 인덱스 또는 -1 (찾지 못함)
 */
int table_find_column(Table *table, const char *col_name);

/**
 * 테이블 정보 조회 (이름, 컬럼 수, 행 수)
 */
void table_get_info(Table *table, char **out_name, size_t *out_cols, size_t *out_rows);

/**
 * 테이블 해제
 */
void table_free(Table *table);

#endif
