// rfc 6455

#ifndef _CX_MAN_h
#define _CX_MAN_h

#include <ctx.h>
#include <stdlib.h>
#include <string.h>

#define CX_MANAGER_BYTE_STREAM_IN 1024
#define CX_MANAGER_CONN_POOL 1024
#define CX_MANAGER_DEALLOC_THRESHOLD 100

enum cxn_state { KEEPALIVE, CLOSE };

typedef struct {
  http_t *http;
  ws_t *ws;
} ctx_t;

typedef struct {
  int fd;
  uint8_t *buf[CX_MANAGER_BYTE_STREAM_IN];
  enum cxn_state reuse;
  ctx_t ctx;
} cxn_ctx_t;

typedef struct {
  cxn_ctx_t **cxn;
  size_t len;
  size_t cap;
  size_t allocated;
} cxn_manager_t;

cxn_manager_t *cxn_manager_create();
// tracks connection
static void cxn_manager_add(cxn_manager_t *cm, int cfd);
// check state of conn add / remove as required
void cxn_manager_track(cxn_manager_t *cm, int cfd);
// releases connection
void cxn_manager_remove(cxn_manager_t *cm, int cfd);
// fetches connection
cxn_ctx_t *cxn_manager_get(cxn_manager_t *cm, int cfd);

#endif
