#ifndef SQL_API_H
#define SQL_API_H

#include "http.h"

/* SQL API 핸들러 */
void handle_sql_execute(int client_fd, HttpRequest *req, AppContext *ctx);

#endif
