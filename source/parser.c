#include "parser.h"
#include "log.h"
#include "hash.h"
#include "http.h"
#include "http_internal.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  NUM_SEMANTIC_TOKENS,
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
  tm_item_t *items[HT_TABLE_SIZE];
  uint8_t *body;
} raw_request_t;

static inline uint32_t validate_path(http_ctx_t *, uint8_t *);
static inline uint32_t validate_method(http_ctx_t *, uint8_t *);
static inline uint32_t validate_version(http_ctx_t *, uint8_t *);
static void parser_parse_http_byte_stream(stream_token_t *, const uint8_t *,
                                          size_t);
static void parser_parse_http_semantics(http_ctx_t *, stream_token_t *, size_t);

static void parser_parse_raw_byte_stream(stream_token_t *stream_tokens,
                                         const uint8_t *stream_in, size_t stream_in_n) {

  if (stream_in_n > MAX_INCOMING_STREAM_SIZE) {
    LOG("stream too large");
    exit(1);
  }

  for (int32_t i = 0; i < stream_in_n; i++) {
    stream_token_t token = {0};
    if (isalpha(stream_in[i])) {
      token.val = stream_in[i];
      token.type = CHAR;
    } else if (isdigit(stream_in[i])) {
      token.val = stream_in[i];
      token.type = NUM;
    } else if (stream_in[i] == '\r') {
      token.val = stream_in[i];
      token.type = CARRIAGE;
    } else if (stream_in[i] == '\n') {
      token.val = stream_in[i];
      token.type = NEWLINE;
    } else if (isblank(stream_in[i])) {
      token.val = stream_in[i];
      token.type = SPACE;
    } else if (stream_in[i] == '/') {
      token.val = stream_in[i];
      token.type = SLASH;
    } else if (stream_in[i] == '.') {
      token.val = stream_in[i];
      token.type = DOT;
    } else if (stream_in[i] == ':') {
      token.val = stream_in[i];
      token.type = COLON;
    } else if (!isalpha(stream_in[i])) {
      token.val = stream_in[i];
      token.type = SPECIAL;
    }
    stream_tokens[i] = token;
  }
}

static inline uint32_t validate_path(http_ctx_t *cx, uint8_t *http_path) {
  if (strcmp((char *)http_path, HTTP_ENDPOINT) == 0) {
    cx->request->path = http_path;
    return 0;
  }
  return 1;
}

static inline http_version_t validate_version(uint8_t *http_version, size_t len) {
  if (http_version == NULL) return HTTP_INVALID;
  if (len == 8 && memcmp(http_version, "HTTP/1.0", 8) == 0) return HTTP_10;
  if (len == 8 && memcmp(http_version, "HTTP/1.1", 8) == 0) return HTTP_11;
  if (len == 6 && memcmp(http_version, "HTTP/2", 6) == 0) return HTTP_2;
  return HTTP_INVALID;
}

static inline uint32_t validate_method(http_ctx_t *cx, uint8_t *m) {
  const char *http_method = (const char *)m;

  const char *http_get_method = "GET";
  const char *http_post_method = "POST";

  if (strcmp(http_method, http_get_method) == 0) {
    cx->request->method = GET;
    return 0;
  } else if (strcmp(http_method, http_post_method) == 0) {
    cx->request->method = POST;
    return 0;
  } else {
    cx->request->method = UNKNOWN;
    return 1;
  }
}

static void parser_parse_http_semantics(http_ctx_t *ctx, stream_token_t *stream_tokens,
                                        size_t stream_token_n) {

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

  int32_t sem_token_idx = 0;

  // don't allocate here
  // potential for area?
  semantic_token_t *semantic_tokens =
      (semantic_token_t *)malloc(sizeof(semantic_token_t) * NUM_SEMANTIC_TOKENS);
  memset(semantic_tokens, 0, sizeof(semantic_token_t) * NUM_SEMANTIC_TOKENS);

  // inc on new req, deal with overriting for keepalive
  tls_inc_map();

  for (int i = 0; i < stream_token_n; i++) {
    stream_token_t *current_token = &stream_tokens[i];

    switch (state) {
    case ERROR_STATE:
        // handle
        return;
        break;
    case IDLE:
      if (current_token->type == CHAR) {
        semantic_tokens[METHOD].val[sem_token_idx++] = current_token->val;
        state = METHOD_STATE;
      } else {
        state = ERROR_STATE;
        // handle
      }
      break;
    case METHOD_STATE:
      semantic_tokens[METHOD].type = METHOD;
      if (current_token->type == CHAR && sem_token_idx <=MAX_HTTP_METHOD_SIZE ) {
        semantic_tokens[METHOD].val[sem_token_idx++] = current_token->val;
      } else if (current_token->type == SPACE) {
        state = PATH_STATE;
        sem_token_idx = 0;

        if (validate_method(ctx, semantic_tokens[METHOD].val) != 0) state = ERROR_STATE;

      } else {
        state = ERROR_STATE;
        // handle method too large
      }
      break;
    case PATH_STATE:
      semantic_tokens[PATH].type = PATH;
      if (current_token->type == SLASH || current_token->type == CHAR) {
        semantic_tokens[PATH].val[sem_token_idx++] = current_token->val;
      } else if (current_token->type == SPACE) {
        state = VERSION_STATE;
        sem_token_idx = 0;

        if (validate_path(ctx, semantic_tokens[PATH].val) != 0) state = ERROR_STATE;
      }
      break;
    case VERSION_STATE:
      semantic_tokens[VERSION].type = VERSION;
      if (current_token->type == CHAR || current_token->type == SLASH ||
          current_token->type == NUM || current_token->type == DOT) {
        semantic_tokens[VERSION].val[sem_token_idx++] = current_token->val;
      } else if (current_token->type == CARRIAGE &&
                 stream_tokens[i + 1].type == NEWLINE) {
        state = HEADER_STATE;
        if (validate_version(ctx, semantic_token[VERSION].val) != 0) state = ERROR_STATE;
        sem_token_idx = 0;
      }
      break;
    case HEADER_STATE:
      semantic_tokens[HEADERS].type = HEADERS;

      // ensure no invalid tokens before COLON
      if (current_token->type == CHAR || current_token->type == COLON ||
          current_token->type == SPACE || current_token->type == NUM ||
          current_token->type == DOT || current_token->type == SLASH ||
          current_token->type == SPECIAL) {
        semantic_tokens[HEADERS].val[sem_token_idx++] = current_token->val;
      } else if (current_token->type == CARRIAGE &&
                 stream_tokens[i + 1].type == NEWLINE &&
                 stream_tokens[i + 2].type == CARRIAGE &&
                 stream_tokens[i + 3].type == NEWLINE) {

        uint8_t *header_stream = semantic_tokens[HEADERS].val;
        size_t h_pos = 0; // overall pointer pos in header stream

        // assumes the correct data is sent
        while (header_stream[h_pos] != '\0') {
            uint8_t tmp_key[MAX_HEADER_KEY_BUF_SIZE] = {0};
            uint8_t tmp_val[MAX_HEADER_VAL_BUF_SIZE] = {0};
            size_t k_pos = 0; // pos in header key
            size_t v_pos = 0; // pos in header val
          while (header_stream[h_pos] != ':') {
            if (k_pos <= MAX_HEADER_KEY_BUF_SIZE)
              tmp_key[k_pos++] = header_stream[h_pos++];
          }
          // skip :<sp>
          if (header_stream[h_pos] == ':') {
            h_pos += 2;
          }
          while (header_stream[h_pos] != ' ' && header_stream[h_pos] != '\0') {
            if (v_pos <= MAX_HEADER_VAL_BUF_SIZE)
              tmp_val[v_pos++] = header_stream[h_pos++];
          }
          // skip <sp>
          if (header_stream[h_pos] == ' ') {
            h_pos++;
          }

          // k,v not allocated yet
          tls_map_insert(tmp_key, strlen(tmp_key), tmp_val);
        }
        // ht_del_hash_table(ht);
        // ctx->request->headers = validate_headers(ht);
        state = BODY_STATE;
        sem_token_idx = 0;
      } else if (current_token->type == CARRIAGE &&
                 stream_tokens[i + 1].type == NEWLINE) {
        semantic_tokens[HEADERS].val[sem_token_idx++] = ' ';
      }
      break;
    case BODY_STATE:
      // depends on headers being accessible via hashmap
      // a conn can remain in BODY_STATE during chunked transfer
      semantic_tokens[BODY].type = BODY;
      if (current_token->type == CHAR) {
        semantic_tokens[BODY].val[sem_token_idx++] = current_token->val;
      }
      ctx->request->body->buf = semantic_tokens[BODY].val;
      // else if sem_token_idx == to be read, state = DONE;
      break;
    case DONE_STATE:
      break;
    default:
      state = DONE_STATE;
    }
  }
}

void parser_parse_http_request(http_ctx_t *ctx, const uint8_t *stream_in,
                               size_t token_count) {
  // don't allocate here
  // max req size is known...
  // can't take size of incomplete type
  stream_token_t *token_stream = calloc(token_count, sizeof(stream_token_t));

  if (token_stream == NULL) {
    perror("never cross the streams!");
    exit(-1);
  }

  parser_parse_raw_byte_stream(token_stream, stream_in, token_count);
  parser_parse_http_semantics(ctx, token_stream, token_count);
  return;
}
