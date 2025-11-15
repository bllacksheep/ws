#ifndef _CTX_h
#define _CTX_h

#include "hash_table.h"
#include <sys/types.h>

typedef enum {
  UNKNOWN,
  GET,
  POST,
} method_t;

typedef struct {
  const uint8_t *name;
  const method_t method;
} method_map_t;

typedef struct {
  uint8_t *body;
  const uint32_t length;
} http_body_t;

typedef struct {
  const method_t request_method;
  const uint8_t *path;
  const uint8_t *version;
  ht_hash_table *headers;
  http_body_t *body;
} http_request_t;

typedef struct {
  const uint8_t *response;
} http_response_t;

typedef struct ws_t ws;

typedef struct {
  http_request_t *request;
  http_response_t *response;
} http_t;

#endif
