#include "parser.h"
#include "hash_table.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void static http_parser_tokenize_byte_stream(stream_token_t *stream,
                                             uint8_t *input, size_t slen) {

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

void static http_parser_tokenize_request_stream(stream_token_t *stream,
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
  printf("method: %s is_method: %d\n", semantic_token[METHOD].val,
         semantic_token[METHOD].type == METHOD);
  printf("path: %s is_path: %d\n", semantic_token[PATH].val,
         semantic_token[PATH].type == PATH);
  printf("version: %s is_version: %d\n", semantic_token[VERSION].val,
         semantic_token[VERSION].type == VERSION);
  printf("headers: %s is_headers: %d\n", semantic_token[HEADERS].val,
         semantic_token[HEADERS].type == HEADERS);
  printf("body: %s is_body: %d\n", semantic_token[BODY].val,
         semantic_token[BODY].type == BODY);
}

// return some high level parsed object that'll fit in a request context
void _parser_parse_http_request(uint8_t *bytes) {
  size_t token_count = strlen(bytes);
  stream_token_t *token_stream =
      (stream_token_t *)malloc(sizeof(stream_token_t) * token_count);
  //
  if (!token_stream) {
    printf("never cross the streams!\n");
    exit(1);
  }

  http_parser_tokenize_byte_stream(token_stream, bytes, token_count);
  //
  //   reflect(tstream, token_count);
  //
  //   tokenize_http_request(tstream, token_count);
}

// int main() {
//
//   // example:
//   //
//   https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers#client_handshake_request
//   uint8_t *req = "GET /chat HTTP/1.1\r\n"
//               "Host: example.com:8000\r\n"
//               "Upgrade: websocket\r\n"
//               "Connection: Upgrade\r\n"
//               "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
//               "Sec-WebSocket-Version: 13\r\n\r\n";
//
//   size_t token_count = strlen(req);
//   stream_token_t *tstream =
//       (stream_token_t *)malloc(sizeof(stream_token_t) * token_count);
//
//   if (!tstream) {
//     printf("bad stream\n");
//     exit(1);
//   }
//
//   tokenize_request_stream(tstream, req, token_count);
//
//   reflect(tstream, token_count);
//
//   tokenize_http_request(tstream, token_count);
// }
