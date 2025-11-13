#include "parser.h"
#include "hash_table.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_parse_http_byte_stream(stream_token_t *stream,
                                          const uint8_t *input, size_t slen) {

  if (slen > MAX_INCOMING_STREAM_SIZE) {
    printf("stream too large!\n");
    exit(1);
  }

  for (int32_t i = 0; i < slen; i++) {
    stream_token_t token;
    if (isalpha(input[i])) {
      token.val = input[i];
      token.type = CHAR;
    } else if (isdigit(input[i])) {
      token.val = input[i];
      token.type = NUM;
    } else if (input[i] == '\r') {
      token.val = input[i];
      token.type = CARRIAGE;
    } else if (input[i] == '\n') {
      token.val = input[i];
      token.type = NEWLINE;
    } else if (isblank(input[i])) {
      token.val = input[i];
      token.type = SPACE;
    } else if (input[i] == '/') {
      token.val = input[i];
      token.type = SLASH;
    } else if (input[i] == '.') {
      token.val = input[i];
      token.type = DOT;
    } else if (input[i] == ':') {
      token.val = input[i];
      token.type = COLON;
    } else if (!isalpha(input[i])) {
      token.val = input[i];
      token.type = SPECIAL;
    }
    stream[i] = token;
  }
}

static raw_request_t *parser_parse_http_req_semantics(stream_token_t *stream,
                                                      size_t token_count) {
  enum {
    IDLE,
    METHOD_STATE,
    PATH_STATE,
    VERSION_STATE,
    HEADER_STATE,
    BODY_STATE,
    DONE_STATE,
    ERROR_STATE,
  } state = IDLE;

  // enum needed to set array size
  enum { MAX_HEADER_BUF = MAX_HEADER_BUF_SIZE };

  int32_t idx = 0;
  semantic_token_t *semantic_token =
      (semantic_token_t *)malloc(sizeof(semantic_token_t) * TOKEN_COUNT);

  memset(semantic_token, 0, sizeof(semantic_token_t) * TOKEN_COUNT);

  ht_hash_table *ht = ht_new();
  // this should be on the heap, freed
  raw_request_t *req;

  for (int32_t i = 0; i < token_count; i++) {
    stream_token_t current_token = stream[i];

    switch (state) {
    case IDLE:
      if (current_token.type == CHAR) {
        semantic_token[METHOD].val[idx++] = current_token.val;
        state = METHOD_STATE;
      }
      break;
    case METHOD_STATE:
      semantic_token[METHOD].type = METHOD;
      if (current_token.type == CHAR) {
        semantic_token[METHOD].val[idx++] = current_token.val;
        // req->method = validate_method(semantic_token[METHOD].val);
      } else if (current_token.type == SPACE) {
        state = PATH_STATE;
        // reset val writer
        idx = 0;
      }
      break;
    case PATH_STATE:
      semantic_token[PATH].type = PATH;
      if (current_token.type == SLASH || current_token.type == CHAR) {
        semantic_token[PATH].val[idx++] = current_token.val;
        // req->path = validate_path(semantic_token[PATH].val);
      } else if (current_token.type == SPACE) {
        state = VERSION_STATE;
        idx = 0;
      }
      break;
    case VERSION_STATE:
      semantic_token[VERSION].type = VERSION;
      if (current_token.type == CHAR || current_token.type == SLASH ||
          current_token.type == NUM || current_token.type == DOT) {
        semantic_token[VERSION].val[idx++] = current_token.val;
        // req->version = validate_version(semantic_token[VERSION].val);
      } else if (current_token.type == CARRIAGE &&
                 stream[i + 1].type == NEWLINE) {
        state = HEADER_STATE;
        idx = 0;
      }
      break;
    case HEADER_STATE:
      semantic_token[HEADERS].type = HEADERS;

      // ensure no invalid tokens before COLON
      if (current_token.type == CHAR || current_token.type == COLON ||
          current_token.type == SPACE || current_token.type == NUM ||
          current_token.type == DOT || current_token.type == SLASH ||
          current_token.type == SPECIAL) {
        semantic_token[HEADERS].val[idx++] = current_token.val;
      } else if (current_token.type == CARRIAGE &&
                 stream[i + 1].type == NEWLINE &&
                 stream[i + 2].type == CARRIAGE &&
                 stream[i + 3].type == NEWLINE) {

        uint8_t key[MAX_HEADER_BUF] = {0};
        uint8_t val[MAX_HEADER_BUF] = {0};
        uint8_t *headers = semantic_token[HEADERS].val;
        int32_t j = 0; // overall pos in headers
        int32_t k = 0;
        int32_t v = 0;

        // assumes the correct data is sent
        while (headers[j] != '\0') {
          while (headers[j] != ':') {
            if (k <= MAX_HEADER_BUF)
              key[k++] = headers[j++];
          }
          // skip :<sp>
          if (headers[j] == ':') {
            j += 2;
          }
          while (headers[j] != ' ' && headers[j] != '\0') {
            if (k <= MAX_HEADER_BUF)
              val[v++] = headers[j++];
          }
          // skip <sp>
          if (headers[j] == ' ') {
            j++;
          }

          ht_insert(ht, key, val);

          memset(key, 0, MAX_HEADER_BUF);
          memset(val, 0, MAX_HEADER_BUF);
          v = k = 0;
        }
        // ht_del_hash_table(ht);
        // req->headers = validate_headers(ht);
        state = BODY_STATE;
        idx = 0;
      } else if (current_token.type == CARRIAGE &&
                 stream[i + 1].type == NEWLINE) {
        semantic_token[HEADERS].val[idx++] = ' ';
      }
      break;
    case BODY_STATE:
      // depends on headers being accessible via hashmap
      // a conn can remain in BODY_STATE during chunked transfer
      semantic_token[BODY].type = BODY;
      if (current_token.type == CHAR) {
        semantic_token[BODY].val[idx++] = current_token.val;
      }
      // else if idx == to be read, state = DONE;
      break;
    case DONE_STATE:
      break;
    default:
      state = DONE_STATE;
    }
  }

  req->method = semantic_token[METHOD].val;
  req->path = semantic_token[PATH].val;
  req->version = semantic_token[VERSION].val;
  req->headers = ht;
  req->body = semantic_token[BODY].val;

  return req;
}

raw_request_t *parser_parse_http_request(const uint8_t *byte_stream) {
  size_t token_count = strlen((const char *)byte_stream);
  stream_token_t *token_stream =
      (stream_token_t *)malloc(sizeof(stream_token_t) * token_count);
  if (!token_stream) {
    printf("never cross the streams!\n");
    exit(1);
  }
  parser_parse_http_byte_stream(token_stream, byte_stream, token_count);
  return parser_parse_http_req_semantics(token_stream, token_count);
}
