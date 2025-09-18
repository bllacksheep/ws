#include "ws-server.h"
#include "http.h"
#include <errno.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
// clean these up check bin size

void handle_conn(unsigned int cfd) {
  while (1) {
    char req[MAX_REQ_SIZE + 1];
    ssize_t bytes_read = read(cfd, req, MAX_REQ_SIZE);
    req[MAX_REQ_SIZE] = '\0';

    if (bytes_read <= 0) {
      if (bytes_read == -1) {
        fprintf(stderr, "error: reading from fd: %d\n", cfd);
        break;
      }
    }
    const char *resp = handle_req(req, bytes_read);

    size_t n = strlen(resp);
    if (n > 0 && write(cfd, resp, n) != (ssize_t)n) {
      fprintf(stderr, "error: writing to fd: %d, %s\n", cfd, strerror(errno));
      break;
    }
  }
  return;
}

int main(int argc, char *argv[]) {

  unsigned int sfd;
  struct sockaddr_in server;
  struct sockaddr_in client;
  struct in_addr ip;

  if (argc < 2) {
    ip.s_addr = htonl(INADDR_LOOPBACK);
  }

  sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  int opt = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

  unsigned int cfd;

  while (1) {
    // deque backlog as client socket
    cfd = accept(sfd, (struct sockaddr *)&client, &cl);
    if (cfd >= 0) {
      fprintf(stdout, "connect accept fd: %d\n", cfd);
    } else {
      fprintf(stderr, "error: accept on: %d, %s\n", cfd, strerror(errno));
      return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
      fprintf(stderr, "error: accept on: %d, %s\n", pid, strerror(errno));
      return 1;
    } else if (pid == 0) {
      close(sfd);
      handle_conn(cfd);
      exit(0);
    } else {
      close(cfd);
    }
  }
  close(sfd);

  return 0;
}
