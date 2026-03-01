#ifndef MYOS_RUNTIME_H
#define MYOS_RUNTIME_H

#include <stddef.h>
#include <stdint.h>
#include "vector.h"
#include "string.h"
#include "list.h"
#include "hash.h"

/* ============================================================================
 * Runtime Context - 모든 로드된 객체 관리
 * ============================================================================ */

typedef struct {
    Vector *vectors;        /* 로드된 Vector 목록 */
    String **strings;       /* 로드된 String 배열 */
    LinkedList **lists;     /* 로드된 LinkedList 배열 */
    HashMap **maps;         /* 로드된 HashMap 배열 */
    
    size_t vector_count;
    size_t string_count;
    size_t list_count;
    size_t map_count;
    
    size_t string_capacity;
    size_t list_capacity;
    size_t map_capacity;
} RuntimeContext;

/* ============================================================================
 * API
 * ============================================================================ */

/**
 * Runtime Context 생성
 */
RuntimeContext* runtime_new(void);

/**
 * Context 해제
 */
void runtime_free(RuntimeContext *ctx);

/**
 * 직렬화 프레임으로부터 Vector 로드
 */
int runtime_load_vector(RuntimeContext *ctx, const uint8_t *frame, size_t len, 
                        size_t *out_id);

/**
 * 직렬화 프레임으로부터 String 로드
 */
int runtime_load_string(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                        size_t *out_id);

/**
 * 직렬화 프레임으로부터 LinkedList 로드
 */
int runtime_load_list(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                      size_t *out_id);

/**
 * 직렬화 프레임으로부터 HashMap 로드
 */
int runtime_load_hashmap(RuntimeContext *ctx, const uint8_t *frame, size_t len,
                         size_t *out_id);

/**
 * 로드된 객체 조회
 */
Vector* runtime_get_vector(RuntimeContext *ctx, size_t id);
String* runtime_get_string(RuntimeContext *ctx, size_t id);
LinkedList* runtime_get_list(RuntimeContext *ctx, size_t id);
HashMap* runtime_get_hashmap(RuntimeContext *ctx, size_t id);

/**
 * 객체 카운트 조회
 */
size_t runtime_vector_count(RuntimeContext *ctx);
size_t runtime_string_count(RuntimeContext *ctx);
size_t runtime_list_count(RuntimeContext *ctx);
size_t runtime_map_count(RuntimeContext *ctx);

#endif
