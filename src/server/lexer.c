#include <ctype.h>
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
} semantic_token_t;

typedef struct {
  char val;
  stream_type_t type;
} stream_token_t;

typedef struct {
  char val[256];
  semantic_token_t type;
} http_token_t;

typedef struct {
  char *headers;
} headers_t;

typedef struct {
  char *data;
} body_t;

typedef struct {
  state_t state;
  semantic_token_t method;
  semantic_token_t path;
  semantic_token_t version;
  headers_t headers;
  unsigned int request_length;
  body_t body;
} ctx;

void tokenize_request_stream(stream_token_t *stream, char *input, size_t slen) {
#define MAX 100

  if (slen > MAX) {
    printf("stream too large!\n");
    exit(1);
  }

  for (int i = 0; i < slen; i++) {
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

void tokenize_http_request(stream_token_t *stream, size_t slen) {

  for (int i = 0; i < slen; i++) {
  }
}

void reflect(stream_token_t *stream, size_t slen) {
  char *types[9] = {
      "CHAR",    "NUM",     "SPACE", "SLASH", "CARRIAGE",
      "NEWLINE", "SPECIAL", "DOT",   "COLON",
  };
  for (int i = 0; i < slen; i++) {
    printf("%s ", types[stream[i].type]);
  }
  putchar('\n');
}

int main() {

  char *req = "GET /chat HTTP/1.1\r\nHost: "
              "127.0.0.1:443\r\nUser-Agent: "
              "curl/7.81.0\r\nAccept: */*\r\n\r\n";

  size_t len = strlen(req);
  stream_token_t *tstream =
      (stream_token_t *)malloc(sizeof(stream_token_t) * len);

  if (!tstream) {
    printf("bad stream\n");
    exit(1);
  }

  tokenize_request_stream(tstream, req, len);

  reflect(tstream, len);

  tokenize_http_request(tstream, len);
}
