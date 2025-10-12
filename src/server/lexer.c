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

typedef enum { METHOD, PATH, VERSION, HEADER } http_type_t;

typedef struct {
  char val;
  stream_type_t type;
} stream_token_t;

stream_token_t *tokenize_request_stream(char *input, size_t len) {
#define MAX 100

  if (len > MAX) {
    printf("stream too large!\n");
    exit(1);
  }

  stream_token_t *token_stream =
      (stream_token_t *)malloc(sizeof(stream_token_t) * len);

  for (int i = 0; i < len; i++) {
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
    token_stream[i] = token;
  }
  return token_stream;
}

void reflect(stream_token_t *tokens, size_t len) {
  char *types[9] = {
      "CHAR",    "NUM",     "SPACE", "SLASH", "CARRIAGE",
      "NEWLINE", "SPECIAL", "DOT",   "COLON",
  };
  for (int i = 0; i < len; i++) {
    printf("%s ", types[tokens[i].type]);
  }
  putchar('\n');
}

int main() {

  char *req = "GET /chat HTTP/1.1\r\nHost: "
              "127.0.0.1:443\r\nUser-Agent: "
              "curl/7.81.0\r\nAccept: */*\r\n\r\n";

  size_t len = strlen(req);
  stream_token_t *stream = tokenize_request_stream(req, len);

  if (!stream) {
    printf("bad stream, couldn't tokenize\n");
    exit(1);
  }

  reflect(stream, len);
}
