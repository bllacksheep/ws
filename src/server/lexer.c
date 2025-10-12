#include <ctype.h>
#include <string.h>

typedef enum {
  CHAR,
  SPACE,
  SLASH,
  CARRIAGE,
  NEWLINE,
  DOT,
  COLON,
} stream_type_t;

typedef enum { METHOD, PATH, VERSION, HEADER } http_type_t;

typedef struct {
  char val;
  stream_type_t type;
} stream_token_t;

void tokenize_request_stream(char *input) {
  stream_token_t token_stream[100] = {0};

  for (int i = 0; i < strlen(input); i++) {
    stream_token_t token;
    if (input[i] == ' ') {
      token.val = input[i];
      token.type = SPACE;
    } else if (input[i] == '/') {
      token.val = input[i];
      token.type = SLASH;
    } else if (input[i] == ':') {
      token.val = input[i];
      token.type = COLON;
    }

    token_stream[i] = token;
  }
}

int main() {

  char *req = "GET /chat HTTP/1.1\r\nHost: 127.0.0.1:443\r\nUser-Agent: "
              "curl/7.81.0\r\nAccept: */*\r\n\r\n";
  tokenize_request_stream(req);
}
