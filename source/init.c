#include "init.h"
#include "cnx_man.h"
#include "hash.h"
#include "ip.h"
#include <arpa/inet.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>


typedef struct server_state s_state_t;

static void server_epoll_create(s_state_t *);
static void server_socket_listen(s_state_t *);
static void server_socket_create(s_state_t *);
static void server_set_listen_addr_port(s_state_t *, char *, char *);
static void server_ipaddrstr_tonetip(s_state_t *);
static void server_set_default_listen_addr_port(s_state_t *);
static void server_ipaddr_tostring(s_state_t *);
static void server_portnum_tostring(s_state_t *);
static void server_hangup(s_state_t *);
static int server_setsock_nonblocking(unsigned int);
static void server_tcp_drain_accept_backlog(int, int,
                              struct epoll_event,
                              cnx_manager_t *, struct sockaddr_in,
                              socklen_t *);

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

static void server_hangup(s_state_t *s) {
  close(s->server_md.fd);
  close(s->epoll_md.fd);
  free(s->cm);
  free(s);
}

static int server_setsock_nonblocking(unsigned int fd) {
  int flags = fcntl(fd, F_GETFL);

  if (!(flags & O_NONBLOCK)) {
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
      perror("could not set O_NONBLOCK on fd");
      return -1;
    }
  }
  return 0;
}

static void server_tcp_drain_accept_backlog(int server_fd, int epoll_fd,
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
        perror("could not accept client connection");
        break;
      }
    }
    cnx_manager_cnx_track(cm, cfd);
    client_event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
    client_event.data.fd = cfd;

    // TODO: add tracked connection object to epoll data
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cfd, &client_event) == -1) {
      perror("could not add client fd to epoll instance list");
    }
  }
}

// string used in log messages
static void server_ipaddr_tostring(s_state_t *s) {
  char buf[20] = {0};
  s->network_md.log_str_ip = strdup(iptoa(DEFAULT_LISTEN_ADDR, buf));
}

// string used in log messages
static void server_portnum_tostring(s_state_t *s) {
  char buf[10] = {0};
  sprintf(buf, "%d", ntohs(s->network_md.server_sin_port));
  s->network_md.log_str_port = strdup(buf);
}

static void server_set_default_listen_addr_port(s_state_t *s) {
  // server_sin_port and addr might be able to retire
  s->network_md.server_sin_port = htons((unsigned short)DEFAULT_LISTEN_PORT);
  s->network_md.server_sin_addr_ip.s_addr = htonl(DEFAULT_LISTEN_ADDR);
  server_ipaddr_tostring(s);
  server_portnum_tostring(s);
}

// convert to useable ip from string
static void server_ipaddrstr_tonetip(s_state_t *s) {
  s->network_md.raw_net_ip =
      atoip(s->network_md.log_str_ip, strlen(s->network_md.log_str_ip));
}

static void server_start_listen_addr_port(s_state_t *s, char *ip, char *port) {
  if (ip == NULL && port == NULL) {
    server_set_default_listen_addr_port(s);
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
    perror("could not create server socket");
    hangup(s);
    exit(-1);
  }

  if (server_setsock_nonblocking(s->server_md.fd) == -1) {
    perror("could not set server sock SOCK_NONBLOCK on fd");
    server_hangup(s);
    exit(-1);
  }

  int opt = 1;
  if (setsockopt(s->server_md.fd, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) == -1) {
    perror("could not set server sock SOL_SOCKET SO_REUSEADDR");
    server_hangup(s);
    exit(-1);
  }
}

// TODO:
//  server size should be stored in server
//  log should be fetched via function
static void server_socket_listen(s_state_t *s) {
  if (bind(s->server_md.fd, (struct sockaddr *)&s->server_md.server,
           sizeof(s->server_md.server)) == -1) {
    perror("could not bind server sock");
    server_hangup(s);
    exit(-1);
  }

  if (listen(s->server_md.fd, s->server_md.listen_backlog) == -1) {
    perror("could not listen server socket");
    server_hangup(s);
    exit(-1);
  }
  printf("Listening on %s:%s\n", s->network_md.log_str_ip,
         s->network_md.log_str_port);
}


static void server_epoll_create(s_state_t *s) {
  if ((s->epoll_md.fd = epoll_create1(0)) == -1) {
    perror("could not create epoll instance");
    server_hangup(s);
    exit(-1);
  }

  s->server_md.events.events = EPOLLIN;
  s->server_md.events.data.fd = s->server_md.fd;

  if (epoll_ctl(s->epoll_md.fd, EPOLL_CTL_ADD, s->server_md.fd,
                &s->server_md.events) == -1) {
    perror("could not add sfd to epoll instance");
    hangup(s);
    exit(-1);
  }
}

// init server, client, epoll here separately
// initialize server state
static s_state_t *server_state_initialization(char *ip, char *port) {
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

  // initialize a connection manager
  init_s_state->cm = cm_allocator();

  if (init_s_state->cm == NULL) {
    fprintf(stderr, "could not create server connection manager");
    exit(-1);
  }

  // bin/server ip port or DEFAULT_LISTEN_ADDR DEFAULT_LISTEN_PORT
  server_start_listen_addr_port(init_s_state, ip, port);

  // initialize local thread storage
  tls_map_init();

  server_socket_create(init_s_state);
  server_socket_listen(init_s_state);
  server_epoll_create(init_s_state);
  return init_s_state;
}

void server_event_loop(s_state_t *s) {
  for (;;) {
    if ((s->epoll_md.n_ready_events = epoll_wait(
             s->epoll_md.fd, s->epoll_md.events, MAX_EVENTS, -1)) == -1) {
      perror("could not wait for server fd on epoll");

      // strace causes EINTR on epoll_wait
      if (errno == EINTR)
        continue;

      server_hangup(s);
      exit(-1);
    }
    for (int n = 0; n < s->epoll_md.n_ready_events; n++) {
      if (s->epoll_md.events[n].events & EPOLLRDHUP) {
        close(s->epoll_md.events[n].data.fd);
        continue;
      }
      if (s->epoll_md.events[n].data.fd == s->server_md.fd) {
        server_tcp_drain_accept_backlog(
            s->server_md.fd, s->epoll_md.fd, s->client_md.events, s->cm,
            s->client_md.client, &s->client_md.client_len);
      } else {
        cm_manage_incoming_cnx(s->cm, s->epoll_md.events[n].data.fd,
                           s->epoll_md.fd);
      }
    }
  }
}

void server_start_event_loop(char* ip, char* port) {
  s_state_t *st = server_state_initialization(ip, port);
  if (st == NULL) {
      fprintf(stderr, "could not create server state");
      server_hangup(st);
      exit(-1);
  }
  server_event_loop(st);
}

void *server_validate_ip_addr(char *ip) {
    return ip; // or NULL
}

void *server_validate_port_addr(char *port) {
    return port; // or NULL
}
