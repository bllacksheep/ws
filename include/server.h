// rfc 6455
#include <stdlib.h>
#include <string.h>

#ifndef _WS_SERVER_h
#define _WS_SERVER_h

#define LISTEN_BACKLOG 20
#define MAX_EVENTS 10
#define PORT 443
#define MAX_BYTE_STREAM_IN 256
#define CONN_MANAGER_CONN_POOL 5

typedef struct {
  int fd;
  char buf[MAX_BYTE_STREAM_IN];
} conn_ctx_t;

typedef struct {
  conn_ctx_t **conn;
  size_t len;
  size_t cap;
} conn_manager_t;

conn_manager_t *connection_manager_create();

conn_manager_t *connection_manager_create() {
  conn_manager_t *cm = (conn_manager_t *)malloc(sizeof(conn_manager_t));
  if (cm == NULL) {
    // handle
  }
  // initial capacity
  cm->conn =
      (conn_ctx_t **)malloc(sizeof(conn_ctx_t *) * CONN_MANAGER_CONN_POOL);

  cm->len = 0;
  cm->cap = CONN_MANAGER_CONN_POOL;
  return cm;
}

// tracks connection
void connection_manager_add(conn_manager_t *cm, int cfd);

void connection_manager_add(conn_manager_t *cm, int cfd) {
  if (cm->len >= cm->cap / 2) {
    cm->cap *= 2;
    cm->conn = (conn_ctx_t **)realloc(cm->conn, sizeof(conn_ctx_t *) * cm->cap);
  }
  conn_ctx_t *conn = (conn_ctx_t *)malloc(sizeof(conn_ctx_t));
  if (conn != NULL) {
    memset(conn, 0, sizeof(conn_ctx_t));
  }
  conn->fd = cfd;
  cm->conn[cfd] = conn;
}

// releases connection
void connection_manager_remove(conn_manager_t *cm, int cfd);
// fetches connection
conn_ctx_t *connection_manager_get(conn_manager_t *cm, int cfd);

#endif
