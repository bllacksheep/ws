#ifndef _HTTP_h
#define _HTTP_h

#include "ctx.h"
#include "parser.h"
#include <stdint.h>

#define MAX_REQ_SIZE 1024
#define ENDPOINT "/chat"

void http_handle_raw_request_stream(ctx_t *);
void http_parse_request(http_t *, const uint8_t *);

#endif
