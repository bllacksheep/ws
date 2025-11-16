#include "conn_man.h"
#include "ctx.h"

#define CNX_MANAGER_BYTE_STREAM_IN 1024

cnx_manager_t *cnx_manager_create() {
  cnx_manager_t *cm = (cnx_manager_t *)malloc(sizeof(cnx_manager_t));
  if (cm == NULL) {
    // handle
  }
  cm->cnx = (ctx_t **)malloc(sizeof(ctx_t *) * CNX_MANAGER_CONN_POOL);

  for (int i = 0; i < CNX_MANAGER_CONN_POOL; i++) {
    ctx_t *cnx = (ctx_t *)malloc(sizeof(ctx_t));
    if (cnx != NULL) {
      memset(cnx, 0, sizeof(ctx_t));
    }
    cm->cnx[i] = cnx;
  }
  cm->len = 0;
  cm->cap = CNX_MANAGER_CONN_POOL;
  cm->allocated = 0;
  return cm;
}

static void cnx_manager_add(cnx_manager_t *cm, int cfd) {
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

  cm->cnx[cfd]->http = (http_t *)malloc(sizeof(http_t));
  if (cm->cnx[cfd]->http == NULL) {
    // handle
  }
  cm->cnx[cfd]->http->request =
      (http_request_t *)malloc(sizeof(http_request_t));
  if (cm->cnx[cfd]->http->request == NULL) {
    // handle
  }

  cm->cnx[cfd]->http->request->path = NULL;
  cm->cnx[cfd]->http->request->version = NULL;
  cm->cnx[cfd]->http->request->headers = NULL;
  cm->cnx[cfd]->http->request->body = NULL;
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

void cnx_manager_track(cnx_manager_t *cm, int cfd) {
  // check the cnx and it's status
  cnx_manager_add(cm, cfd);
}

// releases cnx
void cnx_manager_remove(cnx_manager_t *cm, int cfd) {}
// fetches cnx
// cnx_ctx_t *cnx_manager_get(cnx_manager_t *cm, int cfd) {}
