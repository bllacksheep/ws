#include "http.h"
#include "hash.h"
#include "cnx_internal.h"
#include <parser.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void http_handle_raw_request_stream(http_ctx_t *, const uint8_t *, uint32_t);

typedef enum httpMethods {
  UNKNOWN = 0,
  GET,
  POST,
} http_method_t;

typedef struct httpBody {
  uint8_t *buf;
  const uint32_t length;
} http_body_t;

typedef struct httpResponse {
  // needs response headers here
  const uint8_t *buf;
  const uint32_t length;
} http_response_t;

typedef struct httpRequest {
  http_method_t method;
  uint8_t *path;
  uint8_t *version;
  tm_item_t *headers[HT_TABLE_SIZE];
  http_body_t *body;
} http_request_t;

typedef struct httpCtx {
  http_request_t *request;
  http_response_t *response;
} http_ctx_t;

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


static void http_handle_raw_request_stream(http_ctx_t *ctx, const uint8_t *stream_in, uint32_t stream_n) {
  parser_parse_http_request(ctx, stream_in, stream_n);


  // needs functions for validitity here
  if (ctx->http->request->method == UNKNOWN) {
    fprintf(stderr, "error: initialize request line unknown payload\n");
    ctx->http->response->buf = _400_bad_request;
  }

  if (strcmp((char *)ctx->http->request->path, HTTP_ENDPOINT) != 0) {
    fprintf(stderr, "error: initialize request line uri expectet /chat\n");
    ctx->http->response->buf = _400_bad_request;
  }

  if (ctx->http->request->method != GET) {
    fprintf(stderr, "error: initialize request line not GET method\n");
    ctx->http->response->buf = _405_method_not_allowed;
  }

  cnx->stream_outbuf_n = strlen(cnx->stream_outbuf);
  cnx->stream_outbuf = _200_ok;
}

void http_alloc_buf(cnx_t *cx) {
  http_response_t *response_cont = {0};
  http_request_t *request_cont = {0};
  http_body_t *body_cont = {0};
  http_ctx_t *http_cont = {0};
  uint8_t *body_buf = {0};

  http_cont = calloc(1, sizeof(http_ctx_t));
  if (http == NULL) {
      perror("could not alloc http");
      exit(-1);
  }

  request = calloc(1, sizeof(http_request_t));
  if (request == NULL) {
      perror("could not alloc http request");
      exit(-1);
  }

  body = calloc(1, sizeof(http_body_t));
  if (body == NULL) {
      perror("could not alloc http request body");
      exit(-1);
  }

  body_buf = calloc(RESPONSE_BODY_BUF_SIZE, sizeof(uint8_t));
  if (body_buf == NULL) {
      perror("could not alloc http request body");
      exit(-1);
  }

  response_cont = calloc(1, sizeof(http_response_t));
  if (response == NULL) {
      perror("could not alloc http response");
      exit(-1);
  }

  http->response = response;
  body->buf = body_buf;
  request->body = body;
  http->request = request;

  cnx->http = http;
}

// http can see in internals need to remove
void http_handle_incoming_cnx(cnx_t *cnx) {
  cnx->stream_inbuf_n = recv(cfd, cnx->stream_inbuf, MAX_REQ_SIZE, 0);

  switch (cnx->stream_inbuf_n) {
      case 0:
        if (epoll_ctl(cnx->ev_loop_fd, EPOLL_CTL_DEL, cnx->fd, NULL) == -1) {
            perror("could not remove fd from interest list");
         }
        // 0 EOF == tcp CLOSE_WAIT
        // to free or not to free here cm->cnx[cfd];
        close(cnx->fd);
        break;
      case -1:
      // to free or not to free here cm->cnx[cfd];
      // already closed continue
       return;
       break;
      default:

    http_handle_raw_request_stream(cnx->http,cnx->stream_inbuf, cnx->stream_inbuf_n);

    if (cnx->outbuf > 0) {
      cnx->stream_outbuf_written_n += write(cnx->fd, cnx->stream_outbuf, cnx->stream_outbuf_n);
      if (cnx->stream_outbuf_written_n == -1) {
        perror("could not write on fd");
        return;
      }
      if (cnx->stream_outbuf_written_n != (ssize_t)cnx->stream_outbuf_n) {
        fprintf(stderr, "error: partial write on fd: %d, %s\n", cnx->fd,
                strerror(errno));
        return;
      }
    }
    break;
  }
  return;
}
