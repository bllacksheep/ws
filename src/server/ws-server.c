#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#define LISTEN_BACKLOG 5
#define PORT 443

int
validate_addr(char *ip) {
    // to be implemented
    return 0;
}

int
main(int argc, char *argv[]) {

    unsigned int sfd;
    struct sockaddr_in addr;
    struct in_addr ip;

    if (argc == 2) {
        if (validate_addr(argv[1]) == -1) {
            fprintf(stderr, "invalid ip: %s\n", argv[1]);
        };
        // what type / format is INADDR_LOOPBACK?
        ip.s_addr = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "too many arguments\n");
        return 1;
    } else {
        ip.s_addr = htonl(INADDR_LOOPBACK);
    }

    sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sfd == -1) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET; 
    addr.sin_port = htons((unsigned short)PORT);
    addr.sin_addr = ip;

    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        return 1;
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return 1;
    }

    printf("Listening on 127.0.0.1:%d\n", PORT);

    return 0;
}
