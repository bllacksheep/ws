#include <ctype.h>
#include <string.h>

typedef enum {
  character,
  space,
  slash,
  carriage,
  newline,
  dot,
  colon,
} type_t;

typedef struct {
  char val;
  type_t type;
} token_t;

void tokenize(char *input) {
  token_t tokens[100];

  for (int i = 0; i < strlen(input); i++) {
    token_t token = {0};
    if (input[i] == ' ') {
      token.val = input[i];
      token.type = space;
    } else if (input[i] == '/') {
      token.val = input[i];
      token.type = slash;
    } else if (input[i] == ':') {
      token.val = input[i];
      token.type = colon;
    }

    tokens[i] = token;
  }
}

int main() {

  char *req = "GET /chat HTTP/1.1\r\nHost: 127.0.0.1:443\r\nUser-Agent: "
              "curl/7.81.0\r\nAccept: */*\r\n\r\n";
  tokenize(req);
}
