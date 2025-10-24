// rfc 6455
#include <stdlib.h>

#ifndef _WS_SERVER_h
#define _WS_SERVER_h

#define LISTEN_BACKLOG 20
#define MAX_EVENTS 10
#define PORT 443
#define MAX_BYTE_STREAM_IN 256

typedef struct {
  int fd;
  char buf[MAX_BYTE_STREAM_IN];
} conn_ctx_t;

typedef struct {
  conn_ctx_t *conn;
  size_t len;
  size_t cap;
} connection_manager_t;

#endif
