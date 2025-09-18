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
    unsigned int valid = 0;
    unsigned int invalid = -1;

    if (len < 4) return (raw_req_t){.is_http = invalid}; 

    char *start_req = req;
    char *start_hdp = req;
    char *end_req = req;

    // pos end of request line; terminated \r\n
    for (int i = 0; i < len; i++) {
        if (!(start_hdp[0] == '\r' && 
              start_hdp[1] == '\n')) {
            start_hdp++;
        }
    }

    *start_hdp = '\0';
    // advance to start of headers
    start_hdp += 2;

    for (int i = 0; i < len; i++) {
        if (!(end_req[0] == '\r' &&
               end_req[1] == '\n' &&
               end_req[2] == '\r' &&
               end_req[3] == '\n')) {
           end_req++;
        }
    }

    *end_req = '\0';
    // advance to start of body
    end_req += 4;
    
    // const
    raw_req_t raw = {
        .request_line = start_req,
        .request_headers = start_hdp,
        .request_body = end_req,
        .is_http = valid,
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
parser_basic(const char *raw_buf, int buf_len) {
    raw_req_t raw_req = parse_raw_bytes(raw_buf, buf_len);
    if(raw_req.is_http == -1) return (req_t){ .request_method = UNKNOWN };
    return initialize_request_line(raw_req.request_line);
}

const char *_405_method_not_allowed =
    "\n\nHTTP/1.1 405 Method Not Allowed\r\n"
    "Allow: GET\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 18\r\n\r\n"
    "Method Not Allowed\n";

const char *_400_bad_request =
    "\n\nHTTP/1.1 400 Bad Request\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 13\r\n\r\n"
    "That Ain't It\n";

const char *_200_ok =
    "\n\nHTTP/1.1 200 OK\r\n";

// handler returning response string
const char *
handle_req(const char *raw_req_buf, int raw_buf_len) {
    if (raw_req_buf == NULL || raw_buf_len >= MAX_REQ_SIZE) {
        return NULL;
    }

    req_t req = parser_basic(raw_req_buf, raw_buf_len);

    if (req.request_method == UNKNOWN) {
        fprintf(stderr, "error: initialize request line unknown payload\n");
        return _400_bad_request;
    }

    if (check_path(req.request_uri) != 0) {
        fprintf(stderr, "error: initialize request line uri expectet /chat\n");
        return _400_bad_request;
    }

    if (req.request_method != GET) {
        fprintf(stderr, "error: initialize request line not GET method\n");
        return _405_method_not_allowed;
    }



    // if (req->request_headers != NULL) {
    //     // hash map
    //     printf("%s\n", req->request_headers);
    // }
    //
    // if (req->request_body != NULL) {
    //     // shouldn't be required
    //     printf("%s\n", req->request_body);
    // }



    // validate websocket request headers etc
    // bad request if malformed

    return _200_ok;
}

