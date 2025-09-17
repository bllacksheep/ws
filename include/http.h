#ifndef _HTTP_h
#define _HTTP_h

#define MAX_REQ_SIZE 1024
#define ENDPOINT "/dial"

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
    const char *request_line;
    const char *request_headers;
    const char *request_body;
    const unsigned int is_http;
} raw_req_t;

typedef struct {
    char *body;
    const unsigned int length;
} http_body_t;

typedef enum {
    GET,
    POST,
} method_t;

typedef struct {
    const method_t request_method;
    const char *request_uri;
    const char *request_version;
    ws_headers_t *request_headers;
    http_body_t *request_body;
} req_t;

typedef struct {
    const char *name;
    const method_t method;
} method_map_t;

const char *
handle_req(const char *, int);

#endif
