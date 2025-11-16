#ifndef _CTX_h
#define _CTX_h

#include "hash_table.h"
#include <stdint.h>
#include <sys/types.h>

typedef enum {
  UNKNOWN,
  GET,
  POST,
} method_t;

typedef struct {
  uint8_t *body;
  const uint32_t length;
} http_body_t;

typedef struct {
  const method_t method;
  const uint8_t *path;
  const uint8_t *version;
  ht_hash_table *headers;
  http_body_t *body;
} http_request_t;

typedef struct {
  const uint8_t *buf;
} http_response_t;

typedef struct ws_t ws;

typedef struct {
  http_request_t *request;
  http_response_t *response;
} http_t;

enum cnx_state { KEEPALIVE, CLOSE };

typedef struct {
  int fd;
  uint8_t *buf;
  uint8_t len;
  enum cnx_state reuse;
  http_t *http;
} ctx_t;

#endif
