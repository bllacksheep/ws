#ifndef _HTTP_h
#define _HTTP_h

#include "parser.h"
#include <stdint.h>

#define MAX_REQ_SIZE 1024
#define ENDPOINT "/chat"

// no "update" api so const should be fine
typedef struct {
  const uint8_t *key;
  const uint8_t *value;
} header_item_t;

typedef struct {
  int32_t size;
  int32_t count;
  header_item_t **headers;
} ws_headers_t;

typedef struct {
  const uint8_t *request_line;
  const uint8_t *request_headers;
  const uint8_t *request_body;
  const unsigned int is_http;
} raw_req_t;

typedef struct {
  uint8_t *body;
  const uint32_t length;
} http_body_t;

typedef enum {
  UNKNOWN,
  GET,
  POST,
} method_t;

typedef struct {
  const method_t request_method;
  const uint8_t *request_uri;
  const uint8_t *request_version;
  ws_headers_t *request_headers;
  http_body_t *request_body;
} req_t;

typedef struct {
  const uint8_t *name;
  const method_t method;
} method_map_t;

typedef struct {
  raw_request_t *request;
  const uint8_t *response;
} req_ctx_t;

const req_ctx_t *http_handle_raw_request_stream(const uint8_t *, int32_t);
req_ctx_t *http_parse_request(const uint8_t *);

#endif
