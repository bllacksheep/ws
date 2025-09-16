#ifndef _HTTP_h
#define _HTTP_h

// no "update" api so const should be fine
typedef struct {
    const char *key;
    const char *value;
} header_item_t;

typedef struct {
  int size;
  int count;
  header_item_t **headers;
} ws_headers_t;

typedef struct {
    char *request_line;
    char *request_headers;
    char *request_body;
} raw_req_t;

typedef struct {
    char *body;
    const unsigned int length;
} http_body_t;

typedef struct {
    const int  request_method;
    const char *request_uri;
    ws_headers_t *request_headers;
    http_body_t *request_body;
} req_t;

enum methods {
    GET,
};

char *
handle_req(char *req);

#endif
