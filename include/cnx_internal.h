#ifndef _CNX_INTERNAL_H
#define _CNX_INTERNAL_H 1
#include<sys/types.h>

enum conn_reuse {
  KEEPALIVE = 1,
  CLOSE,
};

enum conn_state {
    UNUSED = 0,
    CLOSED,
    PENDING,
};

// not connected to protocol or http yet
typedef struct conn {
  enum conn_reuse reuse;
  enum conn_state state;
  unsigned int fd;
  unsigned int ev_loop_fd;
  ssize_t stream_out_b_n;
  ssize_t stream_out_b_written_n;
  char *stream_in_b;
  char *stream_out_b;
} cnx_t;

#endif
