#ifndef _IP_h
#define _IP_h

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OCTETS 4
#define MAX_DIGITS_PER_OCTET 3
#define ASCII_RANGE 9
#define BITS 32
#define LEN 15

uint32_t atoip(char *, size_t);

char *iptoa(uint32_t, char *);

#endif
