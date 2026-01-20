#ifndef _WS_SERVER_INIT_H
#define _WS_SERVER_INIT_H 1

#define LISTEN_BACKLOG 1024
#define MAX_EVENTS 1024

void server_start_event_loop(char*, char*);
void *server_validate_port_addr(char *);
void *server_validate_ip_addr(char *);

#endif
