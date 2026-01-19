#ifndef _CNX_MAN_H
#define _CNX_MAN_H 1

#define CNX_MANAGER_CONN_POOL 1024
#define CNX_MANAGER_DEALLOC_THRESHOLD 100
#define CNX_MANAGER_BYTE_STREAM_SIZEIN 1024
#define CNX_MANAGER_BYTE_STREAM_SIZEOUT 1024

#include "cnx.h"

typedef struct conn_man cnx_manager_t;

cnx_manager_t *cm_allocator(void);
cnx_t *cm_get_cnx(cnx_manager_t *, int);
int cm_get_cnx_fd(cnx_t *);
void cm_track_cnx(cnx_manager_t *, int);
void cm_remove_cnx(cnx_t *);

#endif
