#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include "ws-server.h"
// clean these up check bin size


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


int
main(int argc, char *argv[]) {

    unsigned int sfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    struct in_addr ip;

    if (argc < 2) {
        ip.s_addr = htonl(INADDR_LOOPBACK);
    }

    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(
        sfd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );

    if (sfd == -1) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;
    }

    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET; 
    server.sin_port = htons((unsigned short)PORT);
    server.sin_addr = ip;

    if (bind(sfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        return 1;
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return 1;
    }
    printf("Listening on 127.0.0.1:%d\n", PORT);

    memset(&client, 0, sizeof(client));
    socklen_t cl = sizeof(client);
    char req[MAX_REQ_SIZE + 1];

    // deque backlog as client socket
    int cfd = accept(sfd, (struct sockaddr *)&client, &cl);
    if (cfd >= 0) {
        fprintf(stdout, "connect accept fd: %d\n", cfd);
    } else {
        fprintf(stderr, "accept error: %s\n", strerror(errno));
        return 1;
    }

    while (1) {
        ssize_t n = read(cfd, req, MAX_REQ_SIZE);
        if (n == -1) {
            fprintf(stderr, "read error: fd: %d\n", cfd);
         }
        handle_req(req);

        //write(1, req, n);
        //close(cfd);
    }

    return 0;
}
