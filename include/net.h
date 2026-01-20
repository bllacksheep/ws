#ifndef _IP_h
#define _IP_h

#include <stdint.h>

#define NUM_OCTETS 4
#define MAX_DIGITS_PER_OCTET 3
#define ASCII_RANGE 9
#define BITS 32
#define LEN 15
// encryption tbd
#define DEFAULT_LISTEN_PORT 443
#define DEFAULT_LISTEN_ADDR INADDR_LOOPBACK
#define LISTEN_BACKLOG 1024

uint32_t atoip(char *, size_t);

char *iptoa(uint32_t, char *);

#endif
