#include "ws-server.h"
#include "http.h"
#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
// clean these up check bin size

unsigned int handle_conn(unsigned int cfd, unsigned int epfd) {
  char req[MAX_REQ_SIZE + 1] = {0};
  // non-blocking, so should read MAX_REQ_SIZE
  // if data then can't be read until next event
  ssize_t bytes_read = read(cfd, req, MAX_REQ_SIZE);

  // 0 EOF == tcp CLOSE_WAIT
  if (bytes_read == 0) {
    fprintf(stdout, "info: client closed connection: %d\n", cfd);
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL) == -1) {
      fprintf(stderr, "error: remove fd from interest list: %d %s\n", cfd,
              strerror(errno));
    }
    if (close(cfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    return 0;
  } else if (bytes_read == -1) {
    fprintf(stderr, "error: reading from fd: %d\n", cfd);
  } else {
    const char *resp = handle_req(req, bytes_read);

    // partial write handling to be implemented
    size_t n = strlen(resp);
    if (n > 0) {
      ssize_t bytes_written = write(cfd, resp, n);
      if (bytes_written == -1) {
        fprintf(stderr, "error: write failed on fd: %d, %s\n", cfd,
                strerror(errno));
        return -1;
      }
      if (bytes_written != (ssize_t)n) {
        fprintf(stderr, "error: partial write on fd: %d, %s\n", cfd,
                strerror(errno));
        return -1;
      }
    }
  }
  return 0;
}

int setnonblocking(unsigned int fd) {
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

int main(int argc, char *argv[]) {

  unsigned int sfd, cfd, efd, nfds;
  struct sockaddr_in server;
  struct sockaddr_in client;
  struct in_addr ip;

  if (argc < 2) {
    ip.s_addr = htonl(INADDR_LOOPBACK);
  }

  if ((sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    fprintf(stderr, "error: server socket create %s\n", strerror(errno));
    return 1;
  }

  int opt = 1;
  if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    fprintf(stderr, "error: setting sockopts %s\n", strerror(errno));

    if (close(sfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    return 1;
  }

  memset(&server, 0, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = htons((unsigned short)PORT);
  server.sin_addr = ip;

  if (bind(sfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    if (close(sfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    return 1;
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    fprintf(stderr, "listen error: %s\n", strerror(errno));
    if (close(sfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    return 1;
  }

  printf("Listening on 127.0.0.1:%d\n", PORT);

  memset(&client, 0, sizeof(client));
  socklen_t cl = sizeof(client);

  struct epoll_event ev, events[MAX_EVENTS];

  if ((efd = epoll_create1(0)) == -1) {
    fprintf(stderr, "error: createine epoll instance %s\n", strerror(errno));
    if (close(sfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    if (close(efd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    return 1;
  }

  ev.events = EPOLLIN;
  ev.data.fd = sfd;

  if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) == -1) {
    fprintf(stderr, "error: epoll add sfd to instance list failed %s\n",
            strerror(errno));
    if (close(sfd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    if (close(efd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
    }
    return 1;
  }

  for (;;) {
    if ((nfds = epoll_wait(efd, events, MAX_EVENTS, 0)) == -1) {
      fprintf(stderr, "error: epoll_wait %s\n", strerror(errno));
      if (close(sfd) == -1) {
        fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
      }
      if (close(efd) == -1) {
        fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
      }
      return 1;
    }

    for (int n = 0; n < nfds; n++) {
      if (events[n].data.fd == sfd) {
        // deque backlog as client socket
        if ((cfd = accept(sfd, (struct sockaddr *)&client, &cl)) >= 0) {
          fprintf(stdout, "connection accepted on fd: %d\n", cfd);
        } else {
          fprintf(stderr, "error: accept on: %d, %s\n", cfd, strerror(errno));
        }

        // need accept4 to set SOCK_NONBLOCK
        if (setnonblocking(cfd) == -1) {
          if (close(sfd) == -1) {
            fprintf(stderr, "error: close on fd: %d %s\n", cfd,
                    strerror(errno));
          }
          if (close(efd) == -1) {
            fprintf(stderr, "error: close on fd: %d %s\n", cfd,
                    strerror(errno));
          }
          return 1;
        }

        ev.events = EPOLLIN; //| EPOLLET;
        ev.data.fd = cfd;

        if (epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
          fprintf(stderr, "error: epoll add cfd to instance list failed %s\n",
                  strerror(errno));
        }
      } else if (handle_conn(events[n].data.fd, efd) == -1) {
        fprintf(stderr, "error: handle connection error %s\n", strerror(errno));
      }
    }
  }

  if (close(sfd) == -1) {
    fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
  }

  return 0;
}
