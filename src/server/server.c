#include "server.h"
#include "conn_man.h"
#include "ctx.h"
#include "http.h"
#include "hash_table.h"
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


void handle_pending_cxn(cnx_manager_t *cm, unsigned int cfd,
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
    size_t n = strlen((char *)ctx->http->response->buf);
    if (n > 0) {
      ssize_t bytes_written = write(cfd, ctx->http->response->buf, n);
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


void tcp_drain_accept_backlog(int server_fd, int epoll_fd,
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

// create some sub structs here for client server and epoll
typedef struct server_state {
  unsigned int n_recvq_events;
  unsigned int n_ready_events;
  unsigned int sfd;
  unsigned int cfd;
  unsigned int efd;
  unsigned int raw_net_ip;
  unsigned int raw_net_port;
  struct sockaddr_in server;
  struct sockaddr_in client;

  socklen_t client_len;
  struct epoll_event client_events, server_events, events[MAX_EVENTS];

  struct in_addr server_sin_addr_ip;
  in_port_t server_sin_port;

  char *ip_log_string;
  char *port_log_string;

  cnx_manager_t *cm;
} s_state_t;


// string used in log messages
static void ipaddr_tostring(s_state_t *s) {
    char buf[20] = {0};
    s->ip_log_string = iptoa(DEFAULT_LISTEN_ADDR, buf);
}

// string used in log messages
static void portnum_tostring(s_state_t *s) {
    s->port_log_string = ntohs(s->raw_net_port);
}

static void set_default_listen_addr_port(s_state_t *s) {
    // server_sin_port and addr might be able to retire
    s->server_sin_port = htons((unsigned short)DEFAULT_LISTEN_PORT);
    s->server_sin_addr_ip.s_addr = htonl(DEFAULT_LISTEN_ADDR);
    ipaddr_tostring(s);
    portnum_tostring(s);
}

// convert to useable ip from string
static void ipaddrstr_tonetip(s_state_t *s) {
    s->raw_net_ip = atoip(s->ip_log_string, strlen(s->ip_log_string));
}

void set_listen_addr_port(s_state_t *s, char* ip, char* port) {
    if (ip == NULL && port == NULL) {
        set_default_listen_addr_port(s);
    } else {
        s->ip_log_string = ip;
        s->port_log_string = port;

        ipaddrstr_tonetip(s);
        s->server_sin_port = htons((unsigned short)port_log_string);
        s->server_sin_addr_ip.s_addr = htonl(s->raw_net_ip);
    }
    s->server.sin_family = AF_INET;
    s->server.sin_port = s->server_sin_port;
    s->server.sin_addr = s->server_sin_addr_ip;
}


// init server, client, epoll here separately
// initialize server state
s_state_t *server_state_initialization(char *ip, char *port) {
  s_state_t *init_s_state = calloc(1, sizeof(s_state_t);
  init_s_state->listen_backlog = LISTEN_BACKLOG;
  init_s_state->client_len = sizeof(init_s_state->client);

  if (init_s_state == NULL) {
    fprintf(stderr, "could not create server state container");
    exit(-1);
  }

  // initialize connection manager
  init_s_state->cm = cnx_manager_create();

  if (init_s_state->cm == NULL) {
    fprintf(stderr, "could not create server connection manager");
    exit(-1);
  }

  // bin/server ip port or DEFAULT_LISTEN_ADDR DEFAULT_LISTEN_PORT
  set_listen_addr_port(init_s_state, ip, port);

  // initialize local thread storage
  tls_map_init();
  return init_s_state
}

void static hangup(s_state_t* s) {
    close(s->sfd);
    close(s->efd);
    free(s->cm);
    free(s);
}


void server_socket_create(s_state_t *s) {
  if ((s->sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    fprintf(stderr, "could not create server socket %s\n", strerror(errno));
    hangup(s);
    exit(-1);
  }

  if (setnonblocking(s->sfd) == -1) {
    fprintf(stderr, "could not set server sock SOCK_NONBLOCK on fd: %d %s\n", s->sfd,
            strerror(errno));
    hangup(s);
    exit(-1);
  }

  int opt = 1;
  if (setsockopt(s->sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    fprintf(stderr, "could not set server sock SOL_SOCKET SO_REUSEADDR %s\n", strerror(errno));
    hangup(s);
    exit(-1);
  }
}

void server_socket_listen(s_state_t *s) {
  if (bind(s->sfd, (struct sockaddr *)&s->server, sizeof(s->server)) == -1) {
    fprintf(stderr, "could not bind server sock %s\n", strerror(errno));
    hangup(s);
    exit(-1);
  }

  if (listen(ss->sfd, ss->listen_backlog) == -1) {
    fprintf(stderr, "could not listen server socket on %s:%s %s\n", s->ip_log_string, s->port_log_string, strerror(errno));
    hangup(s);
    exit(-1);
  }
  printf("Listening on %s:%d\n", s->ip_log_string, s->port_log_string);
}

int main(int argc, char *argv[]) {
   char *ip = 0;
   char *port = 0;

   if (argc > 2);
      ip = argv[1], port = argv[2];
    
   s_state_t *ss = server_state_initialization(ip, port);

   server_socket_create(ss);
   server_socket_listen(ss);

  if ((ss->efd = epoll_create1(0)) == -1) {
    fprintf(stderr, "could not create epoll instance %s\n",
            strerror(errno));
    hangup(ss);
    exit(-1);
  }

  sev.events = EPOLLIN;
  sev.data.fd = ss->sfd;

  if (epoll_ctl(ss->efd, EPOLL_CTL_ADD, ss->sfd, ss->server_events) == -1) {
    fprintf(stderr, "could not add sfd to epoll instance %s\n",
            strerror(errno));
    hangup(ss);
    exit(-1);
  }
  for (;;) {
    if ((ss->n_ready_events = epoll_wait(ss->efd, ss->events, MAX_EVENTS, -1)) == -1) {
      fprintf(stderr, "could not wait for sfd on epoll intance%s\n", strerror(errno));
      // strace causes EINTR on epoll_wait
      if (errno == EINTR)
        continue;
      hangup(ss);
      exit(-1);
    }
    for (int n = 0; n < ss->n_ready_events; n++) {
      if (ss->events[n].events & EPOLLRDHUP) {
        close(ss->events[n].data.fd);
        continue;
      }
      if (ss->events[n].data.fd == ss->sfd) {
        tcp_drain_accept_backlog(ss->sfd, ss->efd, ss->client_events, ss->cm, ss->client, &ss->client_len);
      } else {
        handle_pending_cxn(ss->cm, ss->events[n].data.fd, ss->efd);
      }
    }
  }
  // shouldn't be hit right now
  hangup(ss);
  return 0;
}
