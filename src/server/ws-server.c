#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#define LISTEN_BACKLOG 1
#define PORT 443

int
validate_addr(char *ip) {
    // to be implemented
    return 0;
}

int
format_addr(char *ip) {
    // to be implemented
    validate_addr(ip);
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
    printf("Listening on 127.0.0.1:%d\n", PORT);
    while (1) {
        int cfd = accept(sfd, (struct sockaddr *)&client, &cl);
        if (cfd == -1) {
            fprintf(stderr, "accept error: %s\n", strerror(errno));
            return 1;
        } else {
            fprintf(stdout, "connect accept on: %d\n", 123);
        }
    }


    return 0;
}
