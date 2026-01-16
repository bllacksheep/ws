#ifndef _PARSER_H
#define _PARSER_H 1
#include<stdint.h>

#define MAX_HEADER_BUF_SIZE 256
#define MAX_SEMANTIC_TOKEN_BUF_SIZE 256
#define MAX_INCOMING_STREAM_SIZE 512

void parser_parse_http_request(const uint8_t *);
#endif
