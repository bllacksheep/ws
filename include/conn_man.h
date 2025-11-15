// rfc 6455

#ifndef _CNX_MAN_h
#define _CNX_MAN_h

#include <ctx.h>
#include <stdlib.h>
#include <string.h>

#define CNX_MANAGER_CONN_POOL 1024
#define CNX_MANAGER_DEALLOC_THRESHOLD 100

typedef struct {
  ctx_t **cnx;
  size_t len;
  size_t cap;
  size_t allocated;
} cnx_manager_t;

cnx_manager_t *cnx_manager_create();
// tracks connection
static void cnx_manager_add(cnx_manager_t *cm, int cfd);
// check state of conn add / remove as required
void cnx_manager_track(cnx_manager_t *cm, int cfd);
// releases connection
void cnx_manager_remove(cnx_manager_t *cm, int cfd);
// fetches connection
ctx_t *cnx_manager_cnx_context(cnx_manager_t *cm, int cfd);

#endif
