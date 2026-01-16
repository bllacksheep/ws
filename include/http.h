#ifndef _HTTP_H
#define _HTTP_H 1

#include "parser.h"
#include <stdint.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_REQ_SIZE 1024
#define RESPONSE_BODY_BUF_SIZE 1024
#define HTTP_ENDPOINT "/chat"

extern typedef struct tm_item tm_item_t;
extern typedef struct conn cnx_t;
typedef enum methods http_method_t;
typedef struct httpBody http_body_t;
typedef struct httpRequest http_request_t;
typedef struct httpResponse http_response_t;
typedef struct ctx ctx_t;

void http_alloc_buf(cnx_t *);
void http_handle_raw_request_stream(ctx_t *);
void http_parse_request(http_ctx_t *, const uint8_t *);



#endif
