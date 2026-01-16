#include "server.h"
#include "init.h"

int main(int argc, char *argv[]) {
  char *ip = 0;
  char *port = 0;

  if (argc > 2) {
    ip = server_validate_ip_addr(argv[1]);
    port = server_validate_port_addr(argv[2]);
  }

  // could take config or options
  server_start_event_loop(ip, port);

  return 0;
}
