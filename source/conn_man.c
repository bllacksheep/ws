#include "conn_man.h"
#include "ctx.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

enum state {
  KEEPALIVE = 1,
  CLOSE,
};

// not connected to protocol or http yet
typedef struct conn {
  enum state reuse;
  unsigned int fd;
  ssize_t len
  char *inbuf;
  char *outbuf;
} cnx_t;

typedef struct conn_man {
  cnx_t **cnx;
  size_t len;
  size_t cap;
  size_t allocated;
} cnx_manager_t;

static void cm_add_cnx(cnx_manager_t *, int);
static void cm_track_cnx(cnx_manager_t *, int);
static void cm_remove_cnx(cnx_manager_t *, int);

cnx_manager_t *cm_create_cm() {
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

// move in buffer from ctx to cnx
// use cnx directly instead of ctx
// add stream len to cnx
// has to go to the parser before it can go anyway not http

void cm_manage_incoming_cnx(cnx_manager_t *cm, unsigned int cfd,
                        unsigned int epfd) {

  cxn_t *cnx = cm->cnx[cfd];
  cnx->fd = cfd;
  ssize_t n = recv(cfd, cnx->buf, MAX_REQ_SIZE, 0);
  cnx->raw_len = n;

  if (bytes_read == 0) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL) == -1) {
      perror("could not remove fd from interest list");
    }
    // 0 EOF == tcp CLOSE_WAIT
    // to free or not to free here cm->cnx[cfd];
    close(cfd);
  } else if (bytes_read == -1) {
    // to free or not to free here cm->cnx[cfd];
    // already closed continue
    return;
  } else {
    /*
     * http validation (supporting small subset)
     * use the parsed input to build a context or return error
     * */
    http_handle_raw_request_stream(ctx);
    // needs to pass to paresr
    // if subset of connman functions are needed as a full wrapper to context
    // don't make a god module... 
    // 

    size_t n = strlen((char *)ctx->http->response->buf);
    if (n > 0) {
      ssize_t bytes_written = write(cfd, ctx->http->response->buf, n);
      if (bytes_written == -1) {
        perror("could not write on fd");
        return;
      }
      if (bytes_written != (ssize_t)n) {
        fprintf(stderr, "error: partial write on fd: %d, %s\n", cfd,
                strerror(errno));
        return;
      }
    }
  }
  return;
}
