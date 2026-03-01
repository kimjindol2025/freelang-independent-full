#include "logs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define INITIAL_CAPACITY 100

// 새 로그 저장소 생성
Logs *logs_new(void) {
    Logs *l = (Logs*)malloc(sizeof(Logs));
    if (!l) return NULL;

    l->entries = (LogEntry*)malloc(sizeof(LogEntry) * INITIAL_CAPACITY);
    if (!l->entries) {
        free(l);
        return NULL;
    }

    l->count = 0;
    l->capacity = INITIAL_CAPACITY;
    return l;
}

// 로그 저장소 해제
void logs_free(Logs *l) {
    if (l) {
        if (l->entries) free(l->entries);
        free(l);
    }
}

// 로그 추가
int logs_add(Logs *l, const char *level, const char *message, const char *username) {
    if (!l || !level || !message) {
        return -1;
    }

    // 용량 확인
    if (l->count >= l->capacity) {
        int new_capacity = l->capacity * 2;
        LogEntry *new_entries = (LogEntry*)realloc(l->entries,
                                                     sizeof(LogEntry) * new_capacity);
        if (!new_entries) {
            return -1;
        }
        l->entries = new_entries;
        l->capacity = new_capacity;
    }

    // 엔트리 추가
    l->entries[l->count].timestamp = time(NULL);
    strncpy(l->entries[l->count].level, level, sizeof(l->entries[l->count].level) - 1);
    strncpy(l->entries[l->count].message, message, sizeof(l->entries[l->count].message) - 1);
    if (username) {
        strncpy(l->entries[l->count].username, username, sizeof(l->entries[l->count].username) - 1);
    }

    l->count++;
    return 0;
}

// 로그 목록 (JSON)
int logs_list(Logs *l, char *out_json, size_t out_size) {
    if (!l || !out_json || out_size < 10) {
        return -1;
    }

    int written = 0;
    written += snprintf(out_json + written, out_size - written, "[");

    // 최근 10개만 (역순)
    int start = (l->count > 10) ? (l->count - 10) : 0;
    int shown = 0;

    for (int i = l->count - 1; i >= start; i--) {
        if (shown > 0) {
            written += snprintf(out_json + written, out_size - written, ",");
        }

        written += snprintf(out_json + written, out_size - written,
                           "{\"timestamp\":%ld,\"level\":\"%s\",\"message\":\"%s\",\"user\":\"%s\"}",
                           l->entries[i].timestamp,
                           l->entries[i].level,
                           l->entries[i].message,
                           l->entries[i].username);

        shown++;

        if (written >= (int)out_size - 10) {
            return -1;  // 버퍼 부족
        }
    }

    written += snprintf(out_json + written, out_size - written, "]");

    return 0;
}
