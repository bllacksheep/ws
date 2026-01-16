enum state {
  KEEPALIVE = 1,
  CLOSE,
};

// not connected to protocol or http yet
typedef struct conn {
  enum state reuse;
  unsigned int fd;
  unsigned int event_loop_fd;
  ssize_t inbuf_n;
  ssize_t outbuf_n;
  ssize_t resp_written_n;
  char *inbuf;
  char *outbuf;
} cnx_t;
