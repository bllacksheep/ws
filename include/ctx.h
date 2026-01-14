#ifndef _CTX_H
#define _CTX_H 1

#include "hash_table.h"
#include <stdint.h>
#include <sys/types.h>

#define ENDPOINT "/chat"

typedef enum methods {
  UNKNOWN,
  GET,
  POST,
} method_t;

typedef struct httpBody {
  uint8_t *buf;
  const uint32_t length;
} http_body_t;

typedef struct httpRequest {
  method_t method;
  uint8_t *path;
  uint8_t *version;
  tm_item_t *headers[TABLE_SIZE];
  http_body_t *body;
} http_request_t;

typedef struct httpResponse {
  const uint8_t *buf;
} http_response_t;

typedef struct {
  http_request_t *request;
  http_response_t *response;
} http_ctx_t;

enum cnx_state { KEEPALIVE, CLOSE };

// needs to be
// handle ctx from connection managers
// add http to it after parse don't define here
//
/*typedef struct ctx {
  connection manager instance
  http_ctx_t *http;
} ctx_t
*/
typedef struct {
  // pointer to connection in manager
  int fd;
  uint8_t *buf;
  uint8_t len;
  enum cnx_state reuse;
  http_ctx_t *http;
} ctx_t;

#endif
