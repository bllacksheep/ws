#ifndef _HTTP_H
#define _HTTP_H 1

#include "cnx.h"
#include <stdint.h>
#include <sys/types.h>

typedef enum httpMethods http_method_t;
typedef enum httpSupportedVersions http_version_t;
typedef struct httpBody http_body_t;
typedef struct httpRequest http_request_t;
typedef struct httpResponse http_response_t;
typedef struct httpCtx http_ctx_t;

void http_destroy(http_ctx_t *);
http_ctx_t *http_alloc_http_ctx();
// void http_parse_request(http_ctx_t *, const uint8_t *, uint32_t,
//                                       const uint8_t *, uint32_t);
void http_handle_incoming_cnx(cnx_t *);
#endif
