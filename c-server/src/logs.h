#ifndef LOGS_H
#define LOGS_H

#include <time.h>

#define MAX_LOG_ENTRIES 1000
#define MAX_LOG_MESSAGE 256

// 로그 엔트리
typedef struct {
    time_t timestamp;
    char level[16];          // INFO, WARN, ERROR
    char message[MAX_LOG_MESSAGE];
    char username[64];       // 누가 했는지
} LogEntry;

// 로그 저장소
typedef struct {
    LogEntry *entries;
    int count;
    int capacity;
} Logs;

// 함수
Logs *logs_new(void);
void logs_free(Logs *l);
int logs_add(Logs *l, const char *level, const char *message, const char *username);
int logs_list(Logs *l, char *out_json, size_t out_size);

#endif
