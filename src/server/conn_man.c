#include "conn_man.h"

conn_manager_t *connection_manager_create() {
  conn_manager_t *cm = (conn_manager_t *)malloc(sizeof(conn_manager_t));
  if (cm == NULL) {
    // handle
  }
  // initial capacity
  cm->conn =
      (conn_ctx_t **)malloc(sizeof(conn_ctx_t *) * CONN_MANAGER_CONN_POOL);

  cm->len = 0;
  cm->cap = CONN_MANAGER_CONN_POOL;
  cm->allocated = 0;
  return cm;
}

static void connection_manager_add(conn_manager_t *cm, int cfd) {
  if (cm->allocated >= CONN_MANAGER_DEALLOC_THRESHOLD) {
    // run in its own thread
    // run cleanup on pool
    // need connection state to be present

    /*
     * responsed means it's done, but keep-alive by default
     * close means it can be removed
     * higher level http status means nothing unless close set
     * so persistent connections can't be cleaned up if still 'managed'
     * meaning realloc is likely unaviodable
     * timeout will have to be set in order to balance performance
     * add idle / stale to conn_ctx_t
     * i.e. persistent but inactive
     * 30s inactive timeout
     *
     *
     * allocating by cfd index is not efficent at scale.... fuck
     *
     */
  }
  // if (cm->len >= cm->cap / 2) {
  //   cm->cap *= 2;
  //   cm->conn = (conn_ctx_t **)realloc(cm->conn, sizeof(conn_ctx_t *) *
  //   cm->cap);
  // }
  conn_ctx_t *conn = (conn_ctx_t *)malloc(sizeof(conn_ctx_t));
  if (conn != NULL) {
    memset(conn, 0, sizeof(conn_ctx_t));
  }
  conn->fd = cfd;
  conn->reuse = CONN_KEEP_ALIVE;
  cm->conn[cfd] = conn;
  cm->len++;
  cm->allocated++;
}

void connection_manager_track(conn_manager_t *cm, int cfd) {
  // check the conn and it's status
  connection_manager_add(cm, cfd);
}

// releases connection
void connection_manager_remove(conn_manager_t *cm, int cfd) {}
// fetches connection
// conn_ctx_t *connection_manager_get(conn_manager_t *cm, int cfd) {}
