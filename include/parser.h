#ifndef _PARSER_H
#define _PARSER_H 1
#include "http.h"
#include<stdint.h>

// max size of a header k,v
#define MAX_HEADER_KEY_BUF_SIZE 256
#define MAX_HEADER_VAL_BUF_SIZE 256
#define MAX_SEMANTIC_TOKEN_BUF_SIZE 256
#define MAX_INCOMING_STREAM_SIZE 512

void parser_parse_http_request(http_ctx_t *, const uint8_t *, size_t);
#endif
