#include "conn_man.h"
#include "cnx_internal.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct conn_man {
  cnx_t **cnx;
  size_t len;
  size_t cap;
  size_t allocated;
} cnx_manager_t;

static void cm_add_cnx(cnx_manager_t *, int);
static void cm_track_cnx(cnx_manager_t *, int);
static void cm_remove_cnx(cnx_manager_t *, int);


static void cm_add_cnx(cm_manage_t *cm) {
  cm->cnx = (cnx_t **)calloc(CNX_MANAGER_CONN_POOL, sizeof(cnx_t *));
  if (cm->cnx == NULL) {
      perror("could not create connections");
      exit(-1);
  }

  for (int i = 0; i < CNX_MANAGER_CONN_POOL; i++) {
    cnx_t *cnx = (cnx_t *)calloc(1, sizeof(cnx_t));
    if (cnx == NULL) {
      perror("could not create cnx connection");
      exit(-1);
    }
    cm->cnx[i] = cnx;
    cm->cnx[i]->reuse = KEEPALIVE;
    cm->cnx[i]->state = UNUSED;
    cm->cnx[i]->inbuf = (uint8_t *)calloc(CNX_MANAGER_BYTE_STREAM_SIZEIN, sizeof(uint8_t));
    if (cm->cnx[i]->inbuf == NULL) {
      perror("could not create connection inbuf");
      exit(-1);
    }
    cm->cnx[i]->outbuf = (uint8_t *)calloc(CNX_MANAGER_BYTE_STREAM_SIZEOUT, sizeof(uint8_t));
    if (cm->cnx[i]->outbuf == NULL) {
      perror("could not create connection outbuf");
      exit(-1);
    }
  }

  http_alloc_buf(cnx);
}

cnx_manager_t *cm_allocator() {
  cnx_manager_t *cm = (cnx_manager_t *)calloc(1, sizeof(cnx_manager_t));
  if (cm == NULL) {
      perror("could not create connection manager");
      exit(-1);
  }
  cm_add_cnx(cm);
  cm->cap = CNX_MANAGER_CONN_POOL;
  cm->allocated = CNX_MANAGER_CONN_POOL;
  return cm;
}

static void cm_remove_cnx(cnx_t *cnx) {
    cnx->state = CLOSED;
}

static void cm_track_cnx(cnx_manager_t *cm, int fd) {
  if (cm == NULL) {
      fprintf(stderr, "no connection manager");
      exit(-1);
  }

  // check if allocated still enough, error count of some kind?
  if (cm->cnx[fd]->state == PENDING) {
      fprintf(stderr, "conn still in use");
      exit(-1);
  }

  cm->cnx[fd]->fd = fd;
  cm->cnx[fd]->state = PENDING;
  cm->len++;


}

