#include <stdio.h>
#include <string.h>
#include "http.h"

char *
check_path(char *uri) {
    char *s, *p;
    p = "/dial";
    // to be implemented
    return 0;
}

// \r\n delimits request line and each header
// \r\n\r\n\n delimtes field line section (headers)
// raw_req_t is const
raw_req_t
parse_raw_bytes(const char *req) {
    char *hdp = req;
    char *terminus = req;

    // set request line ; each line terminated with \r\n
    while (!(hdp[0] == '\r' &&
           hdp[1] == '\n')) {
        hdp++;
        terminus++; // no need to start from beginning
    }

    *hdp = '\0';
    hdp += 2;

    while (!(terminus[0] == '\r' &&
           terminus[1] == '\n' &&
           terminus[2] == '\r' &&
           terminus[3] == '\n')) {
       terminus++;
    }

    *terminus = '\0';
    // advance to start of body
    terminus += 4;
    
    // const
    raw_req_t raw = {
        .request_line = req,
        .request_headers = hdp,
        .request_body = terminus,
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
    char *request_uri = token;

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

    req_t r = {
        .request_method = request_method,
        .request_uri = request_uri,
        .request_version = token,
    };

    return r;
}

// rfc 9110
req_t
parse_raw_request(const raw_req_t *req) {
    // get headers into a hash table. 
    // how I maintain a hash table per request
    // how does this work with threading
    if (req->request_line == NULL) {
        fprintf(stderr, "failed to parse raw request line\n");
    }

    req_t r = initialize_request_line(req->request_line);
    if (r.request_method == -1) {
        fprintf(stderr, "initialize request line error\n");
    }

    if (req->request_headers != NULL) {
        if (r.request_method == POST) {}
        if (r.request_method == GET) {}
        printf("%s\n", req->request_headers);
    }

    if (req->request_body != NULL) {
        printf("%s\n", req->request_body);
    }

    return r;
}

req_t
req_reader(const char *req) {
    raw_req_t raw_req = parse_raw_bytes(req);

    req_t r = parse_raw_request(&raw_req);

    // handle request type
    // handle chunking if body not NULL?
    
    //printf("%s", req);
    return r; 
}


char *
handle_req(const char *req) {
    if (req == NULL) {
        return NULL;
    }
    req_t r = req_reader(req);

    return 0;
}

