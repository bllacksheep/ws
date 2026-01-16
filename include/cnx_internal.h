#ifndef _CNX_INTERNAL_H
#define _CNX_INTERNAL_H 1
#include "http.h"
#include <sys/types.h>

enum conn_reuse {
  KEEPALIVE = 1,
  CLOSE,
};

enum conn_state {
  UNUSED = 0,
  CLOSED,
  PENDING,
};

typedef struct conn {
  enum conn_reuse reuse;
  enum conn_state state;
  unsigned int fd;
  unsigned int ev_loop_fd;
  ssize_t stream_inbuf_n;
  ssize_t stream_outbuf_n;
  ssize_t stream_outbuf_written_n;
  uint8_t *stream_inbuf;
  uint8_t *stream_outbuf;
  http_ctx_t *http;
} cnx_t;

#endif
