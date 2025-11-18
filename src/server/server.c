#include "server.h"
#include "conn_man.h"
#include "ctx.h"
#include "http.h"
#include "ip.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
// clean these up check bin size

void handle_pending_connx(cnx_manager_t *cm, unsigned int cfd,
                          unsigned int epfd) {

  ctx_t *ctx = cm->cnx[cfd];

  ssize_t bytes_read = recvfrom(cfd, ctx->buf, MAX_REQ_SIZE, NULL, NULL, NULL);
  // TODO: if 0 clean up allocated resources
  if (bytes_read == 0) {
    // 0 EOF == tcp CLOSE_WAIT
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL) == -1) {
      fprintf(stderr, "error: remove fd from interest list: %d %s\n", cfd,
              strerror(errno));
    }
    if (close(cfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
      return;
    }
    return;
  } else if (bytes_read == -1) {
    return;
  } else {
    /*
     * read get's you a byte steam
     * make sense of byte steam tokenize handle error states early here
     * validate the lexed content with parsing handle error states here
     *
     * http validation (supporting small subset)
     * use the parsed input to build a context or return error
     * */
    ctx->len = bytes_read;
    http_handle_raw_request_stream(ctx);

    // partial write handling to be implemented
    size_t n = strlen((char *)ctx->http->response);
    if (n > 0) {
      ssize_t bytes_written = write(cfd, ctx->http->response, n);
      if (bytes_written == -1) {
        fprintf(stderr, "error: write failed on fd: %d, %s\n", cfd,
                strerror(errno));
        return;
      }
      if (bytes_written != (ssize_t)n) {
        fprintf(stderr, "error: partial write on fd: %d, %s\n", cfd,
                strerror(errno));
        return;
      }
    }
  }
  return;
}

int static setnonblocking(unsigned int fd) {
  int flags = fcntl(fd, F_GETFL);

  if (!(flags & O_NONBLOCK)) {
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      fprintf(stderr, "error: fnctl setting O_NONBLOCK on fd %d %s\n", fd,
              strerror(errno));
      return -1;
    }
  }
  return 0;
}

void static server_hangup(int server_fd, int epoll_fd) {
  if (close(server_fd) == -1) {
    fprintf(stderr, "error: close on server fd: %d %s\n", server_fd,
            strerror(errno));
  }
  if (epoll_fd > 0) {
    if (close(epoll_fd) == -1) {
      fprintf(stderr, "error: close on epoll fd: %d %s\n", epoll_fd,
              strerror(errno));
    }
  }
}

void drain_tcp_accept_backlog(int server_fd, int epoll_fd,
                              struct epoll_event client_event,
                              cnx_manager_t *cm, struct sockaddr_in client_addr,
                              socklen_t *client_addr_len) {
  int cfd;
  for (;;) {
    // deque backlog as client socket
    cfd = accept4(server_fd, (struct sockaddr *)&client_addr, client_addr_len,
                  SOCK_NONBLOCK);
    if (cfd < 0) {
      // drained
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
        // feels unnecessary
      } else {
        fprintf(stderr, "error: client accept on: %d, %s\n", cfd,
                strerror(errno));
        break;
      }
    }
    cnx_manager_cnx_track(cm, cfd);
    client_event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    client_event.data.fd = cfd;

    // TODO: add tracked connection object to epoll data
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &client_event) == -1) {
      fprintf(stderr, "error: epoll add cfd to instance list failed %s\n",
              strerror(errno));
    }
  }
}

int main(int argc, char *argv[]) {
  unsigned int num_recv_q_events, num_ready_events;
  unsigned int sfd, cfd, efd;
  struct sockaddr_in server;
  struct sockaddr_in client;
  struct in_addr ip;
  unsigned int rawip = 0;
  char *address;
  in_port_t port;

  cnx_manager_t *cnxmgr = cnx_manager_create();
  if (cnxmgr == NULL) {
    // handle
  }

#define DEFAULT INADDR_LOOPBACK

  // bin/server 127.0.0.1 8080
  if (argc < 2) {
    ip.s_addr = htonl(DEFAULT);
    // convert to string
    char buf[20] = {0};
    address = iptoa(DEFAULT, buf);
    port = htons((unsigned short)PORT);
  }
  // all other args discarded
  if (argc >= 3) {
    address = argv[1];
    // convert to useable
    rawip = atoip(address, strlen(address));
    ip.s_addr = htonl(rawip);
    unsigned short port_atoi = atoi(argv[2]);
    port = htons((unsigned short)port_atoi);
  }

  if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    fprintf(stderr, "error: server socket create %s\n", strerror(errno));
    return 1;
  }

  if (setnonblocking(sfd) == -1) {
    fprintf(stderr, "error: setting SOCK_NONBLOCK on fd: %d %s\n", cfd,
            strerror(errno));
    server_hangup(sfd, 0);
    // potentially some form of exit
  }

  int opt = 1;
  if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    fprintf(stderr, "error: setting sockopts %s\n", strerror(errno));
    server_hangup(sfd, 0);
    return 1;
  }

  memset(&server, 0, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = port;
  server.sin_addr = ip;

  if (bind(sfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    server_hangup(sfd, 0);
    return 1;
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    fprintf(stderr, "listen error: %s\n", strerror(errno));
    server_hangup(sfd, 0);
    return 1;
  }

  printf("Listening on %s:%d\n", address, ntohs(port));

  memset(&client, 0, sizeof(client));
  socklen_t cl = sizeof(client);
  struct epoll_event cev, sev, events[MAX_EVENTS];

  if ((efd = epoll_create1(0)) == -1) {
    fprintf(stderr, "error: creating server conn epoll instance %s\n",
            strerror(errno));
    server_hangup(sfd, efd);
    return 1;
  }

  sev.events = EPOLLIN;
  sev.data.fd = sfd;

  if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &sev) == -1) {
    fprintf(stderr, "error: epoll add sfd to instance list failed %s\n",
            strerror(errno));
    server_hangup(sfd, efd);
    return 1;
  }
  for (;;) {
    if ((num_ready_events = epoll_wait(efd, events, MAX_EVENTS, -1)) == -1) {
      fprintf(stderr, "error: epoll_wait on server fd %s\n", strerror(errno));
      // strace causes EINTR on epoll_wait
      if (errno == EINTR)
        continue;
      server_hangup(sfd, efd);
      exit(1);
    }
    for (int n = 0; n < num_ready_events; n++) {
      if (events[n].events & EPOLLRDHUP) {
        close(events[n].data.fd);
        continue;
      }
      if (events[n].data.fd == sfd) {
        drain_tcp_accept_backlog(sfd, efd, cev, cnxmgr, client, &cl);
      } else {
        handle_pending_connx(cnxmgr, events[n].data.fd, efd);
      }
    }
  }
  if (close(sfd) == -1) {
    fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
  }
  return 0;
}
