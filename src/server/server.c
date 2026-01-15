#include "server.h"
#include "conn_man.h"
#include "ctx.h"
#include "hash_table.h"
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

void handle_pending_cxn(cnx_manager_t *cm, unsigned int cfd,
                        unsigned int epfd) {

  ctx_t *ctx = cm->cnx[cfd];

  ssize_t bytes_read = recv(cfd, ctx->buf, MAX_REQ_SIZE, 0);
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

struct epoll_metadata {
  struct epoll_event events[MAX_EVENTS];
  unsigned int n_recvq_events;
  unsigned int n_ready_events;
  unsigned int fd;
};

struct client_metadata {
  struct epoll_event events;
  struct sockaddr_in client;
  socklen_t client_len;
  unsigned int fd;
};

struct server_metadata {
  struct epoll_event events;
  unsigned int listen_backlog;
  struct sockaddr_in server;
  unsigned int fd;
};

struct network_metadata {
  struct in_addr server_sin_addr_ip;
  in_port_t server_sin_port;
  unsigned int raw_net_ip;
  unsigned int raw_net_port;
  char *log_str_ip;
  char *log_str_port;
};

typedef struct server_state {
  struct epoll_metadata epoll_md;
  struct client_metadata client_md;
  struct server_metadata server_md;
  struct network_metadata network_md;

  cnx_manager_t *cm;
} s_state_t;

static void hangup(s_state_t *s) {
  close(s->server_md.fd);
  close(s->epoll_md.fd);
  free(s->cm);
  free(s);
}

// string used in log messages
static void ipaddr_tostring(s_state_t *s) {
  char buf[20] = {0};
  s->network_md.log_str_ip = strdup(iptoa(DEFAULT_LISTEN_ADDR, buf));
}

// string used in log messages
static void portnum_tostring(s_state_t *s) {
  char buf[10] = {0};
  sprintf(buf, "%d", ntohs(s->network_md.server_sin_port));
  s->network_md.log_str_port = strdup(buf);
}

static void set_default_listen_addr_port(s_state_t *s) {
  // server_sin_port and addr might be able to retire
  s->network_md.server_sin_port = htons((unsigned short)DEFAULT_LISTEN_PORT);
  s->network_md.server_sin_addr_ip.s_addr = htonl(DEFAULT_LISTEN_ADDR);
  ipaddr_tostring(s);
  portnum_tostring(s);
}

// convert to useable ip from string
static void ipaddrstr_tonetip(s_state_t *s) {
  s->network_md.raw_net_ip =
      atoip(s->network_md.log_str_ip, strlen(s->network_md.log_str_ip));
}

static void set_listen_addr_port(s_state_t *s, char *ip, char *port) {
  if (ip == NULL && port == NULL) {
    set_default_listen_addr_port(s);
  } else {
    s->network_md.log_str_ip = ip;
    s->network_md.log_str_port = port;

    ipaddrstr_tonetip(s);
    unsigned short port_atoi = atoi(port);
    s->network_md.server_sin_port = htons(port_atoi);
    s->network_md.server_sin_addr_ip.s_addr = htonl(s->network_md.raw_net_ip);
  }
  s->server_md.server.sin_family = AF_INET;
  s->server_md.server.sin_port = s->network_md.server_sin_port;
  s->server_md.server.sin_addr = s->network_md.server_sin_addr_ip;
}

static void server_socket_create(s_state_t *s) {
  if ((s->server_md.fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    fprintf(stderr, "could not create server socket %s\n", strerror(errno));
    hangup(s);
    exit(-1);
  }

  if (setnonblocking(s->server_md.fd) == -1) {
    fprintf(stderr, "could not set server sock SOCK_NONBLOCK on fd: %d %s\n",
            s->server_md.fd, strerror(errno));
    hangup(s);
    exit(-1);
  }

  int opt = 1;
  if (setsockopt(s->server_md.fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) == -1) {
    fprintf(stderr, "could not set server sock SOL_SOCKET SO_REUSEADDR %s\n",
            strerror(errno));
    hangup(s);
    exit(-1);
  }
}

// TODO:
//  server size should be stored in server
//  log should be fetched via function
static void server_socket_listen(s_state_t *s) {
  if (bind(s->server_md.fd, (struct sockaddr *)&s->server_md.server,
           sizeof(s->server_md.server)) == -1) {
    fprintf(stderr, "could not bind server sock %s\n", strerror(errno));
    hangup(s);
    exit(-1);
  }

  if (listen(s->server_md.fd, s->server_md.listen_backlog) == -1) {
    fprintf(stderr, "could not listen server socket on %s:%s %s\n",
            s->network_md.log_str_ip, s->network_md.log_str_port,
            strerror(errno));
    hangup(s);
    exit(-1);
  }
  printf("Listening on %s:%s\n", s->network_md.log_str_ip,
         s->network_md.log_str_port);
}

static void server_epoll_create(s_state_t *s) {
  if ((s->epoll_md.fd = epoll_create1(0)) == -1) {
    fprintf(stderr, "could not create epoll instance %s\n", strerror(errno));
    hangup(s);
    exit(-1);
  }

  s->server_md.events.events = EPOLLIN;
  s->server_md.events.data.fd = s->server_md.fd;

  if (epoll_ctl(s->epoll_md.fd, EPOLL_CTL_ADD, s->server_md.fd,
                &s->server_md.events) == -1) {
    fprintf(stderr, "could not add sfd to epoll instance %s\n",
            strerror(errno));
    hangup(s);
    exit(-1);
  }
}

// init server, client, epoll here separately
// initialize server state
s_state_t *server_state_initialization(char *ip, char *port) {
  // init_server()
  // init_event_loop()
  // init_client()
  // init_tls()
  // init_tls_map()
  s_state_t *init_s_state = calloc(1, sizeof(s_state_t));
  init_s_state->server_md.listen_backlog = LISTEN_BACKLOG;
  init_s_state->client_md.client_len = sizeof(init_s_state->client_md.client);

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
  // probably not a good place to init this but needed for now
  tls_map_init();

  server_socket_create(init_s_state);
  server_socket_listen(init_s_state);
  server_epoll_create(init_s_state);
  return init_s_state;
}


void server_state_start(char* ip, char* port) {
  s_state_t *s = server_state_initialization(ip, port);

  for (;;) {
    if ((s->epoll_md.n_ready_events = epoll_wait(
             s->epoll_md.fd, s->epoll_md.events, MAX_EVENTS, -1)) == -1) {
      fprintf(stderr, "could not wait for sfd on epoll intance%s\n",
              strerror(errno));
      // strace causes EINTR on epoll_wait
      if (errno == EINTR)
        continue;
      hangup(s);
      exit(-1);
    }
    for (int n = 0; n < s->epoll_md.n_ready_events; n++) {
      if (s->epoll_md.events[n].events & EPOLLRDHUP) {
        close(s->epoll_md.events[n].data.fd);
        continue;
      }
      if (s->epoll_md.events[n].data.fd == s->server_md.fd) {
        tcp_drain_accept_backlog(
            s->server_md.fd, s->epoll_md.fd, s->client_md.events, s->cm,
            s->client_md.client, &s->client_md.client_len);
      } else {
        handle_pending_cxn(s->cm, s->epoll_md.events[n].data.fd,
                           s->epoll_md.fd);
      }
    }
  }
}

void *validate_ip_addr(char *ip) {
    return ip;
}

void *validate_port_addr(char *port) {
    return port;
}

int main(int argc, char *argv[]) {
  char *ip = 0;
  char *port = 0;

  if (argc > 2) {
    ip = validate_ip_addr(argv[1]);
    port = validate_port_addr(argv[2]);
  }

  server_state_start(ip, port);

  return 0;
}
