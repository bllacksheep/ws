#ifndef _WS_SERVER_INIT_H
#define _WS_SERVER_INIT_H 1

#define LISTEN_BACKLOG 1024
#define MAX_EVENTS 1024
// encryption tbd
#define DEFAULT_LISTEN_PORT 443
#define DEFAULT_LISTEN_ADDR INADDR_LOOPBACK

void server_state_start(char*, char*);
void *validate_port_addr(char *);
void *validate_ip_addr(char *);

#endif
