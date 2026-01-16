#include "conn_man.h"
#include "cnx_internal.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

typedef struct conn_man {
  cnx_t **cnx;
  size_t len;
  size_t cap;
  size_t allocated;
} cnx_manager_t;

static void cm_add_cnx(cnx_manager_t *, int);
static void cm_track_cnx(cnx_manager_t *, int);
static void cm_remove_cnx(cnx_manager_t *, int);

cnx_manager_t *cm_allocator() {
  cnx_manager_t *cm = (cnx_manager_t *)malloc(sizeof(cnx_manager_t));
  if (cm == NULL) {
      perror("could not create connection manager");
      exit(-1);
  }
  cm->cnx = (cxn_t **)malloc(sizeof(cxn_t *) * CNX_MANAGER_CONN_POOL);

  for (int i = 0; i < CNX_MANAGER_CONN_POOL; i++) {
    cnx_t *cnx = (cxn_t *)calloc(1, sizeof(cxn_t));
    cm->cnx[i] = cnx;
  }
  cm->len = 0;
  cm->cap = CNX_MANAGER_CONN_POOL;
  cm->allocated = 0;
  return cm;
}

static void cm_add_cnx(cnx_manager_t *cm, int cfd) {
  if (cm->allocated >= CNX_MANAGER_DEALLOC_THRESHOLD) {
  }
  if (cm->cnx[cfd] == NULL) {
    // handle
  }
  cm->cnx[cfd]->fd = cfd;
  cm->cnx[cfd]->reuse = KEEPALIVE;
  cm->len++;
  cm->allocated++;

  cm->cnx[cfd]->buf =
      (uint8_t *)malloc(sizeof(uint8_t) * CNX_MANAGER_BYTE_STREAM_IN);

  if (cm->cnx[cfd]->buf == NULL) {
    // handle
  }

  cm->cnx[cfd]->http = (http_ctx_t *)malloc(sizeof(http_ctx_t));
  if (cm->cnx[cfd]->http == NULL) {
    // handle
  }
  cm->cnx[cfd]->http->request =
      (http_request_t *)malloc(sizeof(http_request_t));
  if (cm->cnx[cfd]->http->request == NULL) {
    // handle
  }

  cm->cnx[cfd]->http->request->body =
      (http_body_t *)malloc(sizeof(http_body_t));
  if (cm->cnx[cfd]->http->request->body == NULL) {
    // handle
  }

// play values to be adjusted
#define BODY_BUF_SIZE 1024

  cm->cnx[cfd]->http->request->body->buf =
      (uint8_t *)malloc(sizeof(uint8_t) * BODY_BUF_SIZE);
  if (cm->cnx[cfd]->http->request->body->buf == NULL) {
    // handle
  }

  // cm->cnx[cfd]->http->request->path = NULL;
  // cm->cnx[cfd]->http->request->version = NULL;
  // cm->cnx[cfd]->http->request->headers = NULL;
  // cm->cnx[cfd]->http->request->body->buf = NULL;
  cm->cnx[cfd]->http->response =
      (http_response_t *)malloc(sizeof(http_response_t));
  if (cm->cnx[cfd]->http->response == NULL) {
    // handle
  }
  /*
   * responsed means it's done, but keep-alive by default
   * close means it can be removed
   * higher level http status means nothing unless close set
   * so persistent cnxs can't be cleaned up if still 'managed'
   * meaning realloc is likely unaviodable
   * timeout will have to be set in order to balance performance
   * add idle / stale to cnx_ctx_t
   * i.e. persistent but inactive
   * 30s inactive timeout
   *
   *
   * allocating by cfd index is not efficent at scale.... fuck
   *
   */
  // if (cm->len >= cm->cap / 2) {
  //   cm->cap *= 2;
  //   cm->cnx = (cnx_ctx_t **)realloc(cm->cnx, sizeof(cnx_ctx_t *) *
  //   cm->cap);
  // }

  //   memset(cnx, 0, sizeof(cnx_ctx_t));
  // }
}

static void cm_remove_cnx(cnx_manager_t *cm, int cfd) {}
