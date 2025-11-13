#ifndef _Parser_h
#define _Parser_h

#include "hash_table.h"
#include <stdint.h>
#include <sys/types.h>
// not optimized

#define MAX_HEADER_BUF_SIZE 256
#define MAX_SEMANTIC_TOKEN_BUF_SIZE 256
#define MAX_INCOMING_STREAM_SIZE 512

typedef enum {
  CHAR,
  NUM,
  SPACE,
  SLASH,
  CARRIAGE,
  NEWLINE,
  SPECIAL,
  DOT,
  COLON,
} stream_type_t;

typedef enum {
  METHOD,
  PATH,
  VERSION,
  HEADERS,
  BODY,
  TOKEN_COUNT
} semantic_type_t;

typedef struct {
  uint8_t val;
  stream_type_t type;
} stream_token_t;

typedef struct {
  uint8_t val[MAX_SEMANTIC_TOKEN_BUF_SIZE];
  semantic_type_t type;
} semantic_token_t;

typedef struct {
  uint8_t *headers;
} headers_t;

typedef struct {
  uint8_t *data;
} body_t;

typedef struct {
  uint8_t *method;
  uint8_t *path;
  uint8_t *version;
  ht_hash_table *headers;
  uint8_t *body;
} raw_request_t;

static void parser_parse_http_byte_stream(stream_token_t *, const uint8_t *,
                                          size_t);
static raw_request_t *parser_parse_http_req_semantics(stream_token_t *, size_t);
raw_request_t *parser_parse_http_request(const uint8_t *);
#endif
