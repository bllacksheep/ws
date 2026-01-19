#ifndef HTTP_INTERNAL_H
#define HTTP_INTERNAL_H 1
#include <hash.h>
#include <stdint.h>

#define MAX_REQ_SIZE 1024
#define REQUEST_BODY_BUF_SIZE 1024
#define RESPONSE_BODY_BUF_SIZE 1024
#define HTTP_ENDPOINT "/chat"

typedef struct httpBody {
  uint8_t *buf;
  const uint32_t length;
} http_body_t;

typedef struct httpResponse {
  tm_item_t *headers[HT_TABLE_SIZE];
  http_body_t *body;
} http_response_t;

typedef struct httpRequest {
  http_method_t method;
  uint8_t *path;
  uint8_t *version;
  tm_item_t *headers[HT_TABLE_SIZE];
  http_body_t *body;
} http_request_t;

typedef struct httpCtx {
  http_request_t *request;
  http_response_t *response;
} http_ctx_t;
#endif
