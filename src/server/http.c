#include <stdio.h>
#include "http.h"

char *
check_path(char *uri) {
    char *s, *p;
    p = "/dial";
    // to be implemented
    return 0;
}

raw_req_t
parse_raw_bytes(char *req) {
    // \n denotes requset line end
    // \r\n denotes header end
    // header end to end denotes end
    char *hdp = req;
    char *terminus = req;

    raw_req_t raw;
    raw.request_line = req;

    // set request line ; each line terminated with \r\n
    while (!(hdp[0] == '\r' &&
           hdp[1] == '\n')) {
        hdp++;
        terminus++; // no need to start from beginning
    }

    *hdp = '\0';
    hdp += 2;

    raw.request_headers = hdp;

    while (!(terminus[0] == '\r' &&
           terminus[1] == '\n' &&
           terminus[2] == '\r' &&
           terminus[3] == '\n')) {
       terminus++;
    }

    *terminus = '\0';
    // advance to start of body
    terminus += 4;
    
    raw.request_body = terminus;

    printf("%s\n", raw.request_line);
    printf("%s\n", raw.request_headers);
    printf("%s\n", raw.request_body);

    return raw;

}

req_t
parse_raw_request(raw_req_t *req) {

    req_t r;
    return r;
}

req_t
req_reader(char *req) {
    raw_req_t raw_req = parse_raw_bytes(req);

    req_t r = parse_raw_request(&raw_req);
    //printf("%s", req);
    return r; 
}


char *
handle_req(char *req) {
    if (req == NULL) {
        return NULL;
    }

    req_t r = req_reader(req);

    return 0;
}

