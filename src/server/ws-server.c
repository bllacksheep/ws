#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#define LISTEN_BACKLOG 5
#define MAX_REQ_SIZE 200
#define PORT 443

typedef struct {
  char *host;
  char *upgrade;
  char *connection;
  char *seckey;
  char *secver; 
} headers_t;

typedef struct {
    char *method;
    char *path;
    headers_t *headers;
} req_t;

char *
check_path(char *path) {
    // to be implemented
    return 0;
}

req_t *
req_reader(char *req) {
    // if path == dial
    printf("full request");
    printf(req);
    return 0;
}


char *
handle_req(char *req) {

    if (req == NULL) {
        return NULL;
    }
    
    req_t *r = req_reader(req);
    
    if (r != NULL) {
        if (check_path(r->path)) {
            // other validations
            return "upgrade true";
         }
    }

    return 0;
}

int
main(int argc, char *argv[]) {

    unsigned int sfd;
    struct sockaddr_in server;
    struct sockaddr_in client;
    struct in_addr ip;

    // if (argc == 2) {
    //     if (validate_addr(argv[1]) == -1) {
    //         fprintf(stderr, "invalid ip: %s\n", argv[1]);
    //     };
    //     // what type / format is INADDR_LOOPBACK?
    //     ip.s_addr = argv[1];
    // } else if (argc > 2) {
    //     fprintf(stderr, "too many arguments\n");
    //     return 1;
    // } else {
    //     ip.s_addr = htonl(INADDR_LOOPBACK);
    // }
    //
    
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


    memset(&client, 0, sizeof(client));
    socklen_t cl = sizeof(client);
    char req[MAX_REQ_SIZE + 1];

    printf("Listening on 127.0.0.1:%d\n", PORT);
    while (1) {
        // deque backlog as client socket 
        int cfd = accept(sfd, (struct sockaddr *)&client, &cl);

        if (cfd >= 0) {
            fprintf(stdout, "connect accept fd: %d\n", cfd);
            ssize_t n = read(cfd, req, MAX_REQ_SIZE);

            if (n < MAX_REQ_SIZE) {
                req[n] = '\n';
            }
            
            handle_req(req);

            write(1, req, MAX_REQ_SIZE);
        } else {
            fprintf(stderr, "accept error: %s\n", strerror(errno));
            return 1;
        }

        close(cfd);
    }


    return 0;
}
