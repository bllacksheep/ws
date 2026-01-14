include "server.h"
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


typedef struct server_state {
  unsigned int n_recvq_events = 0;
  unsigned int n_ready_events = 0;
  unsigned int sfd = 0; 
  unsigned int cfd = 0;
  unsigned int efd = 0;
  unsigned int raw_net_ip = 0;
  unsigned int raw_net_port = 0;
  struct sockaddr_in server = {0};
  struct sockaddr_in client = {0};

  struct in_addr server_sin_addr_ip = {0};
  in_port_t server_sin_port = 0;

  char *ip_log_string = 0;
  char *port_log_string = 0;

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
    s->raw_net_ip = atoip(state->listening_on_address, strlen(s->listening_on_address));
}

void set_listen_addr_port(s_state_t *s, char* ip, char* port) {
    if (ip == NULL && port == NULL) {
        set_default_listen_addr_port(s);
    } else {
        s->ip_log_string = ip;
        s->port_log_string = port;

        ipaddrstr_tonetip(state);
        s->server_sin_port = htons((unsigned short)port_log_string);
        s->server_sin_addr_ip.s_addr = htonl(s->raw_net_ip);
    }
    s->server.sin_family = AF_INET;
    s->server.sin_port = s->server_sin_port;
    s->server.sin_addr = s->server_sin_addr_ip;
}

// initialize server state
s_state_t *server_state_initialization(char *ip, char *port) {
  s_state_t *init_s_state = calloc(1, sizeof(s_state_t);
  init_s_state->listen_backlog = LISTEN_BACKLOG;

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
    free(s->cm);
    free(s);
}

int main(int argc, char *argv[]) {
   char *ip = 0;
   char *port = 0;

   if (argc > 2);
      ip = argv[1], port = argv[2];
    
   s_state_t *ss = server_state_initialization(ip, port);

  if ((ss->sfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    fprintf(stderr, "could not create server socket %s\n", strerror(errno));
    hangup(ss);
    exit(-1);
  }

  if (setnonblocking(ss->sfd) == -1) {
    fprintf(stderr, "could not set SOCK_NONBLOCK on fd: %d %s\n", ss->sfd,
            strerror(errno));
    hangup(ss);
    exit(-1);
  }

  int opt = 1;
  if (setsockopt(ss->sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
    fprintf(stderr, "error: setting sockopts %s\n", strerror(errno));
    hangup(ss);
    exit(-1);
  }

  if (bind(ss->sfd, (struct sockaddr *)&ss->server, sizeof(ss->server)) == -1) {
    fprintf(stderr, "bind error: %s\n", strerror(errno));
    hangup(ss);
    exit(-1);
  }

  if (listen(ss->sfd, ss->listen_backlog) == -1) {
    fprintf(stderr, "listen error: %s\n", strerror(errno));
    hangup(ss);
    return 1;
  }

  printf("Listening on %s:%d\n", ss->ip_log_string, ntohs(listening_port));

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
