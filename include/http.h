#ifndef _HTTP_h
#define _HTTP_h

typedef struct {
  char *host;
  char *upgrade;
  char *connection;
  char *seckey;
  char *secver;
} ws_headers_t;

typedef struct {
    char *request_line;
    char *request_headers;
    char *request_body;
} raw_req_t;

typedef struct {
    int  request_method;
    char *request_uri;
    ws_headers_t *request_headers;
} req_t;

enum methods {
    GET,
};

#endif
