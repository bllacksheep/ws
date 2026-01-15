#ifndef _CNX_MAN_H
#define _CNX_MAN_H 1

#define CNX_MANAGER_CONN_POOL 1024
#define CNX_MANAGER_DEALLOC_THRESHOLD 100
#define CNX_MANAGER_BYTE_STREAM_IN 1024

typedef struct conn cnx_t;
typedef struct conn_man cnx_manager_t;

cnx_manager_t *cm_create_cm(void);
cnx_t *cm_get_cnx(cnx_manager_t *, int);
void cm_manage_incnx(cnx_manager_t *, unsigned int, unsigned int);

#endif
