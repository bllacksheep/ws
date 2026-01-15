#ifndef _HTTP_H
#define _HTTP_H 1

#include "ctx.h"
#include "parser.h"
#include <stdint.h>

#define MAX_REQ_SIZE 1024

void http_handle_raw_request_stream(ctx_t *);
void http_parse_request(http_ctx_t *, const uint8_t *);

#endif
