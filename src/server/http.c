#include <stdio.h>
#include <string.h>
#include "http.h"

unsigned int
check_path(const char *uri) {
    if (strcmp(uri, ENDPOINT) == 0) {
        return 0;
    }    
    return -1;
}

// in lieu of a real lexer/parser to be implemented
// \r\n delimits request line and each header
// \r\n\r\n\n delimtes field line section (headers)
// raw_req_t is const
raw_req_t
parse_raw_bytes(const char *req, int len) {
    // no \r\n\r\n at least, kill it.
    // if (len <= 4) {
    //     return (raw_req_t){ .is_http = -1 };
    // }

    char *hdp = req;
    char *terminus = req;

    // set request line ; each line terminated with \r\n
    for (int i = 0; i < len; i++) {
        if (!(hdp[0] == '\r' && 
              hdp[1] == '\n')) {
            hdp++;
        }
    }

    *hdp = '\0';
    hdp += 2;

    // if garbase terminated by \r\n validate 

    for (int i = 0; i < len; i++) {
        if (!(terminus[0] == '\r' &&
               terminus[1] == '\n' &&
               terminus[2] == '\r' &&
               terminus[3] == '\n')) {
           terminus++;
        }
    }


    *terminus = '\0';
    // advance to start of body
    terminus += 4;
    
    // const
    raw_req_t raw = {
        .request_line = req,
        .request_headers = hdp,
        .request_body = terminus,
        .is_http = 0,
    };

    return raw;

}

req_t
initialize_request_line(const char *rl) {

    char *token = rl;
    char *req_meth_str = rl;
    method_t request_method;

    static const method_map_t methods[] = {
        {.name = "GET", .method = GET},
        {.name = "POST", .method = POST},
    };

    while (token[0] != ' ') {
        token++;
    }

    token[0] = '\0';
    token++;
    const char *request_uri = token;

    for (size_t i = 0; i < sizeof(methods)/sizeof(methods[0]); i++) {
        if (strcmp(methods[i].name, req_meth_str) == 0) {
            request_method = methods[i].method;
        }
    }

    while (token[0] != ' ') {
        token++;
    }

    token[0] = '\0';
    token++;
    
    const char* request_version = token;

    req_t r = {
        .request_method = request_method,
        .request_uri = request_uri,
        .request_version = request_version,
    };

    return r;
}

// rfc 9110
req_t
validate_http_request(const raw_req_t *req) {
    // get headers into a hash table. 
    // how I maintain a hash table per request
    // how does this work with threading

    req_t r = initialize_request_line(req->request_line);

    if (r.request_method == -1) {
        fprintf(stderr, "error: initialize request line unknown payload\n");
        return r;
    }

    if (r.request_method != GET) {
        fprintf(stderr, "error: initialize request line not GET method\n");
        return r;
    }

    if (check_path(r.request_uri) != 0) {
        fprintf(stderr, "error: initialize request line not /dial uri\n");
        return r;
    }

    if (req->request_headers != NULL) {
        // hash map
        printf("%s\n", req->request_headers);
    }

    if (req->request_body != NULL) {
        // shouldn't be required
        printf("%s\n", req->request_body);
    }

    return r;
}

req_t
parse_and_validate_http_request(const char *raw_buf, int buf_len) {
    raw_req_t raw_req = parse_raw_bytes(raw_buf, buf_len);
    if(raw_req.is_http == -1) return (req_t){};

    return validate_http_request(&raw_req);
}

const char *_405_method_not_allowed =
    "HTTP/1.1 405 Method Not Allowed\r\n"
    "Allow: GET\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 18\r\n\r\n"
    "Method Not Allowed";

const char *_400_bad_request =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 18\r\n\r\n"
    "that ain't it";

// handler returning response string
const char *
handle_req(const char *raw_req_buf, int raw_buf_len) {
    if (raw_req_buf == NULL || raw_buf_len >= MAX_REQ_SIZE) {
        return NULL;
    }

    req_t req = parse_and_validate_http_request(raw_req_buf, raw_buf_len);

    if (req.request_method == -1) {
        return _400_bad_request;
    }

    if (req.request_method != GET) {
        return _405_method_not_allowed;
    }

    // validate websocket request headers etc
    // bad request if malformed

    return NULL;
}

