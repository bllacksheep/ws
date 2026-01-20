#include "http.h"
#include "log.h"
#include "cnx_internal.h"
#include "http_internal.h"
#include "http_responses.h"
#include <errno.h>
#include <parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

static void http_handle_raw_request_stream(http_ctx_t *, const uint8_t *,
                                           size_t,
                                           uint8_t *,
                                           size_t *);


static void http_handle_raw_request_stream(http_ctx_t *ctx,
                                           const uint8_t *stream_inbuf,
                                           size_t stream_inbuf_n,
                                           uint8_t *stream_outbuf,
                                           size_t *stream_outbuf_n) {

  parser_parse_http_request(ctx, stream_inbuf, stream_inbuf_n);

  // needs response + headers + body

  if (ctx->request->method == UNKNOWN) {
    LOG("initialize request line unknown payload");
    memcpy(stream_outbuf, _400_bad_request, strlen(_400_bad_request));
    *stream_outbuf_n = strlen(stream_outbuf);
  }

  if (memcmp(ctx->request->path, HTTP_ENDPOINT, 5) != 0) {
    LOG("initialize request line uri expects '/chat'");
    memcpy(stream_outbuf, _400_bad_request, strlen(_400_bad_request));
    *stream_outbuf_n = strlen(stream_outbuf);
  }

  if (ctx->request->method != GET) {
    LOG("initialize request line not GET method");
    memcpy(stream_outbuf, _405_method_not_allowed, strlen(_405_method_not_allowed));
    *stream_outbuf_n = strlen(stream_outbuf);
  }

  memcpy(stream_outbuf, _200_ok, strlen(_200_ok));
  *stream_outbuf_n = strlen(stream_outbuf);
}

http_ctx_t *http_alloc_http_ctx(void) {
  http_ctx_t *ctx = calloc(1, sizeof(*ctx));
  if (ctx == NULL) {
    LOG("could not alloc http");
    perror("could not alloc http");
    exit(-1);
  }

  ctx->request = calloc(1, sizeof(*ctx->request));
  if (ctx->request == NULL) {
    LOG("could not alloc http request");
    perror("could not alloc http request");
    exit(-1);
  }

  ctx->response = calloc(1, sizeof(*ctx->response));
  if (ctx->response == NULL) {
    LOG("could not alloc http response");
    perror("could not alloc http response");
    exit(-1);
  }

  ctx->request->body = calloc(1, sizeof(*ctx->request->body));
  if (ctx->request->body == NULL) {
    LOG("could not alloc http request body");
    perror("could not alloc http request body");
    exit(-1);
  }

  ctx->response->body = calloc(1, sizeof(*ctx->response->body));
  if (ctx->response->body == NULL) {
    LOG("could not alloc http response body");
    perror("could not alloc http response body");
    exit(-1);
  }

  ctx->request->body->buf = calloc(REQUEST_BODY_BUF_SIZE, sizeof(*ctx->request->body->buf));
  if (ctx->request->body->buf == NULL) {
      LOG("could not alloc http request body buf");
      perror("could not alloc http request body buf");
    exit(-1);
  }

  ctx->response->body->buf = calloc(RESPONSE_BODY_BUF_SIZE, sizeof(*ctx->response->body->buf));
  if (ctx->response->body->buf == NULL) {
    LOG("could not alloc http response body buf");
    perror("could not alloc http response body buf");
    exit(-1);
  }

  return ctx;
}

void http_destroy(http_ctx_t *http) {
    if (http == NULL) return;
    if (http->request) {
        if (http->request->body) {
            free(http->request->body->buf);
            free(http->request->body);
        }
        free(http->request);
    }

    if (http->response) {
        if (http->response->body) {
            free(http->response->body->buf);
            free(http->response->body);
        }
    free(http->response);
    }
    free(http);
}


void http_handle_incoming_cnx(cnx_t *cx) {

  // check here if cx is existing conn or new cx
  ssize_t stream_n = recv(cx->fd, cx->stream_inbuf, MAX_REQ_SIZE, 0);

  if (stream_n <= 0) {
    LOG("no bytes read for stream_inbuf");
    if (epoll_ctl(cx->ev_loop_fd, EPOLL_CTL_DEL, cx->fd, NULL) == -1) {
      LOG("could not remove fd from interest list");
    }
    // 0 EOF == tcp CLOSE_WAIT
    // to free or not to free here cm->cnx[cfd];
    close(cx->fd);
  }

    cx->stream_inbuf_n = stream_n;

    http_handle_raw_request_stream(cx->http, cx->stream_inbuf,
                                   cx->stream_inbuf_n,
                                   cx->stream_outbuf,
                                   &cx->stream_outbuf_n);

    if (cx->stream_outbuf_n > 0) {
      cx->stream_outbuf_written_n += write(cx->fd, cx->stream_outbuf, cx->stream_outbuf_n);
      if (cx->stream_outbuf_written_n == -1) {
        LOG("could not write on fd");
        close(cx->fd);
      }
      if (cx->stream_outbuf_written_n != (ssize_t)cx->stream_outbuf_n) {
        LOG("could not partial write on fd");
        fprintf(stderr, "error: partial write on fd: %d, %s\n", cx->fd,
                strerror(errno));
        return;
        }
      }
  return;
}
