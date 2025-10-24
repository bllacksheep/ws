// rfc 6455
#include <stdlib.h>
#include <string.h>

#ifndef _CONN_MAN_h
#define _CONN_MAN_h

#define MAX_BYTE_STREAM_IN 256
#define CONN_MANAGER_CONN_POOL 10

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
// tracks connection
static void connection_manager_add(conn_manager_t *cm, int cfd);
// check state of conn add / remove as required
void connection_manager_track(conn_manager_t *cm, int cfd);
// releases connection
void connection_manager_remove(conn_manager_t *cm, int cfd);
// fetches connection
conn_ctx_t *connection_manager_get(conn_manager_t *cm, int cfd);

#endif
