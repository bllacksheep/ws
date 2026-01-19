#ifndef _HTTP_H
#define _HTTP_H 1

#include "cnx.h"
#include <stdint.h>
#include <sys/types.h>

#define MAX_REQ_SIZE 1024
#define RESPONSE_BODY_BUF_SIZE 1024
#define HTTP_ENDPOINT "/chat"

typedef enum httpMethods http_method_t;
typedef struct httpBody http_body_t;
typedef struct httpRequest http_request_t;
typedef struct httpResponse http_response_t;
typedef struct httpCtx http_ctx_t;

typedef enum httpMethods {
  UNKNOWN = 0,
  GET,
  POST,
} http_method_t;

void http_alloc_buf(cnx_t *);
// void http_parse_request(http_ctx_t *, const uint8_t *, uint32_t,
//                                       const uint8_t *, uint32_t);
void http_handle_incoming_cnx(cnx_t *);
#endif
