#include "server.h"
#include "conn_man.h"
#include "http.h"
#include "ip.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
// clean these up check bin size

// handle conn is not a loop and it needs to be
void handle_conn(conn_manager_t *cm, unsigned int cfd, unsigned int epfd) {
  // printf("entered handle_conn: fd %d\n", cfd);

  // char http_request_stream[MAX_REQ_SIZE + 1] = {0};
  // non-blocking, so should read MAX_REQ_SIZE
  // if data then can't be read until next event
  int x = 1;
  ssize_t bytes_read = read(cfd, cm->conn[cfd]->buf, MAX_REQ_SIZE);

  // printf("bytes read: %zd: fd %d\n", bytes_read, cfd);

  // TODO: if 0 clean up allocated resources
  if (bytes_read == 0) {
    // 0 EOF == tcp CLOSE_WAIT
    // fprintf(stdout, "info: client closed connection: %d\n", cfd);
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
    fprintf(stderr, "error: reading from fd: %d, %s\n", cfd, strerror(errno));
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

    // assign context to connection
    const char *http_response_stream =
        handle_http_request_stream(cm->conn[cfd]->buf, bytes_read);

    // const char *http_response_stream =
    //     handle_http_request_stream(http_request_stream, bytes_read);

    // partial write handling to be implemented
    size_t n = strlen(http_response_stream);
    if (n > 0) {
      ssize_t bytes_written = write(cfd, http_response_stream, n);
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

int connection_error(int fd, int epfd) {
  int err = 0;
  socklen_t len = sizeof(err);
  if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1) {
    fprintf(stderr, "error: getting socket opts, fd: %d %s\n", fd,
            strerror(errno));
    return -1;
  }
  if (err == ECONNRESET) {
    // printf("Connection reset by peer\n");

    if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
      fprintf(stderr, "error: removing stale fd from interest list: %d %s\n",
              fd, strerror(errno));
      return -1;
    }
    if (close(fd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", fd, strerror(errno));
      return -1;
    }
    return -1;
  } else if (err != 0) {
    fprintf(stderr, "error: unknown socket error %d %s\n", fd, strerror(errno));
    if (close(fd) == -1) {
      fprintf(stderr, "error: close on fd: %d %s\n", fd, strerror(errno));
      return -1;
    }
    return -1;
  }
  return 0;
}

void server_shutdown(int server_fd, int server_epoll_fd, int client_epoll_fd) {
  if (close(server_fd) == -1) {
    fprintf(stderr, "error: close on server fd: %d %s\n", server_fd,
            strerror(errno));
  }
  if (server_epoll_fd > 0) {
    if (close(server_epoll_fd) == -1) {
      fprintf(stderr, "error: close on epoll fd: %d %s\n", server_epoll_fd,
              strerror(errno));
    }
  }
  if (client_epoll_fd > 0) {
    if (close(client_epoll_fd) == -1) {
      fprintf(stderr, "error: close on epoll fd: %d %s\n", client_epoll_fd,
              strerror(errno));
    }
  }
}

void drain_accept_queue(int server_fd, int server_epoll_fd, int client_epoll_fd,
                        struct epoll_event client_event,
                        conn_manager_t *connection_manager,
                        struct sockaddr_in client_addr,
                        socklen_t *client_addr_len) {

  int cfd;
  for (;;) {
    // deque backlog as client socket
    cfd = accept(server_fd, (struct sockaddr *)&client_addr, client_addr_len);
    if (cfd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      } else {
        fprintf(stderr, "error: client accept on: %d, %s\n", cfd,
                strerror(errno));
        break;
      }
    }

    // need accept4() to set SOCK_NONBLOCK flag
    if (setnonblocking(cfd) == -1) {
      fprintf(stderr, "error: setting SOCK_NONBLOCK on fd: %d %s\n", cfd,
              strerror(errno));
      server_shutdown(server_fd, server_epoll_fd, client_epoll_fd);
      // potentially some form of exit
    }

    connection_manager_track(connection_manager, cfd);
    //
    // for (int i = 5; i < conn_mgr->cap; i++) {
    //   if (conn_mgr->conn[i] != NULL) {
    //     printf("fd: %d, buf: %s, ds cap: %zu\n", conn_mgr->conn[i]->fd,
    //            conn_mgr->conn[i]->buf, conn_mgr->cap);
    //   }
    // }

    client_event.events = EPOLLIN | EPOLLERR; //| EPOLLET;
    client_event.data.fd = cfd;

    if (epoll_ctl(client_epoll_fd, EPOLL_CTL_ADD, cfd, &client_event) == -1) {
      fprintf(stderr, "error: epoll add cfd to instance list failed %s\n",
              strerror(errno));
    }
  }
}

int main(int argc, char *argv[]) {

  unsigned int num_recv_q_events, num_data_ready_events;
  unsigned int sfd, cfd, sefd, cefd;
  struct sockaddr_in server;
  struct sockaddr_in client;
  struct in_addr ip;
  unsigned int rawip = 0;
  char *address;
  in_port_t port;

  conn_manager_t *conn_mgr = connection_manager_create();
  if (conn_mgr == NULL) {
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
    server_shutdown(sfd, 0, 0);
    // potentially some form of exit
  }

  int opt = 1;
  if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    fprintf(stderr, "error: setting sockopts %s\n", strerror(errno));
    server_shutdown(sfd, 0, 0);
    return 1;
  }

  memset(&server, 0, sizeof(server));

  server.sin_family = AF_INET;
  server.sin_port = port;
  server.sin_addr = ip;

  if (bind(sfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    server_shutdown(sfd, 0, 0);
    return 1;
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    fprintf(stderr, "listen error: %s\n", strerror(errno));
    server_shutdown(sfd, 0, 0);
    return 1;
  }

  printf("Listening on %s:%d\n", address, ntohs(port));

  memset(&client, 0, sizeof(client));
  socklen_t cl = sizeof(client);
  struct epoll_event sev, server_events[MAX_EVENTS];
  struct epoll_event cev, client_events[MAX_EVENTS];

  if ((sefd = epoll_create1(0)) == -1) {
    fprintf(stderr, "error: creating server conn epoll instance %s\n",
            strerror(errno));
    server_shutdown(sfd, sefd, 0);
    return 1;
  }

  if ((cefd = epoll_create1(0)) == -1) {
    fprintf(stderr, "error: creating client conn epoll instance %s\n",
            strerror(errno));
    server_shutdown(sfd, sefd, cefd);
    return 1;
  }

  sev.events = EPOLLIN | EPOLLERR;
  sev.data.fd = sfd;

  if (epoll_ctl(sefd, EPOLL_CTL_ADD, sfd, &sev) == -1) {
    fprintf(stderr, "error: epoll add sfd to instance list failed %s\n",
            strerror(errno));
    server_shutdown(sfd, sefd, cefd);
    return 1;
  }

  void *t(void *data) {
    pthread_t tid;

    tid = pthread_self();
    printf("thread id: %ld\n", tid);

    for (;;) {
      if (epoll_wait(sefd, server_events, MAX_EVENTS, -1) == -1) {
        fprintf(stderr, "error: epoll_wait on server fd %s\n", strerror(errno));
        // strace causes EINTR on epoll_wait
        if (errno == EINTR)
          continue;
        server_shutdown(sfd, sefd, cefd);
        exit(1);
      }
      drain_accept_queue(sfd, sefd, cefd, cev, conn_mgr, client, &cl);
    }
  }

  pthread_t accept_loop;
  pthread_t conn_loop;

  pthread_create(&accept_loop, NULL, t, NULL);

  for (;;) {
    // will be on its own thread
    if ((num_data_ready_events =
             epoll_wait(cefd, client_events, MAX_EVENTS, -1)) == -1) {
      fprintf(stderr, "error: epoll_wait on client fd %s\n", strerror(errno));
      // strace causes EINTR on epoll_wait
      if (errno == EINTR)
        continue;
      server_shutdown(sfd, sefd, cefd);
      return 1;
    }
    for (int n = 0; n < num_data_ready_events; n++) {
      if (!connection_error(client_events[n].data.fd, cefd)) {
        handle_conn(conn_mgr, client_events[n].data.fd, cefd);
      }
    }
  }

  if (close(sfd) == -1) {
    fprintf(stderr, "error: close on fd: %d %s\n", cfd, strerror(errno));
  }

  return 0;
}
