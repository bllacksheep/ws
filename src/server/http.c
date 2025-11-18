#include "http.h"
#include "ctx.h"
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int check_path(const uint8_t *uri) {
  if (strcmp((char *)uri, ENDPOINT) == 0) {
    return 0;
  }
  return -1;
}

const uint8_t *_405_method_not_allowed =
    "\n\nHTTP/1.1 405 Method Not Allowed\r\n"
    "Allow: GET\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 18\r\n\r\n"
    "Method Not Allowed\n";

const uint8_t *_400_bad_request = "\n\nHTTP/1.1 400 Bad Request\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 13\r\n\r\n"
                                  "That Ain't It\n";

const uint8_t *_200_ok = "HTTP/1.1 200 OK\r\n"
                         "Server: server/0.0.0 (Ubuntu)\r\n"
                         "Content-Length: 0\r\n"
                         "Date: Thu, 06 Nov 2025 21:58:42 GMT\r\n"
                         "Content-Type: text/plain\r\n"
                         "Connection: keep-alive\r\n\r\n";

void http_parse_request(http_t *ctx, const uint8_t *stream) {
  parser_parse_http_request(ctx, stream);
}

void http_handle_raw_request_stream(ctx_t *ctx) {
  if (ctx->buf == NULL || (int)ctx->len >= MAX_REQ_SIZE) {
    exit(1);
  }
  http_parse_request(ctx->http, ctx->buf);

  if (ctx->http->request->method == UNKNOWN) {
    fprintf(stderr, "error: initialize request line unknown payload\n");
    ctx->http->response->buf = _400_bad_request;
  }

  if (check_path(ctx->http->request->path) != 0) {
    fprintf(stderr, "error: initialize request line uri expectet /chat\n");
    ctx->http->response->buf = _400_bad_request;
  }

  if (ctx->http->request->method != GET) {
    fprintf(stderr, "error: initialize request line not GET method\n");
    ctx->http->response->buf = _405_method_not_allowed;
  }

  ctx->http->response->buf = _200_ok;
}
