#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_OCTETS 4
#define MAX_DIGITS_PER_OCTET 3
#define ASCII_RANGE 9
#define BITS 32
#define LEN 15

// doing this sucks without a parser :/
uint32_t atoip(char *in_ipstr, size_t ipstr_len) {

  char ipstr[20] = {0};
  uint32_t next_octet = 24;
  uint32_t curr_octet = 0;
  uint32_t pos = 0;
  uint32_t acc = 0;
  uint32_t ip = 0;
  uint32_t m = 100;

  for (int i = 0; i < ipstr_len + 1; i++) {
    ipstr[i] = in_ipstr[i];
  }

  // terminating '.'
  ipstr[ipstr_len] = '.';

  for (int i = 0; i <= ipstr_len + 1; i++) {

    if (ipstr[i] == '.') {

      // shift right if required
      switch (pos) {
      case 1:
        curr_octet /= 100;
        break;
      case 2:
        curr_octet /= 10;
        break;
      default:
        break;
      }

      if (curr_octet != 0)
        ip += curr_octet << next_octet;

      next_octet -= 8;

      curr_octet = 0;
      pos = 0;
      acc = 0;
      m = 100;
      continue;
    }

    acc = (uint32_t)ipstr[i] - '0';
    acc *= m;
    m /= 10;
    curr_octet += acc;
    // how many digits were there in the octet
    pos++;
  }
  printf("%d\n", ip);
  return ip;
}

// output { '1', '2', '7' '.' '0' '.' '0' '.' '1' '\0'}
void iptoa(uint32_t ip) {
  char str[LEN] = {0};
  uint32_t curr_octet, next_octet = BITS;
  uint32_t oct_pos, str_pos = 0;
  uint32_t tot_octet = 0;

  while (tot_octet < NUM_OCTETS) {
    // for octets > '9'
    char tmp_octet[MAX_DIGITS_PER_OCTET + 1] = {0};

    oct_pos = 3;
    next_octet -= 8;

    curr_octet = (ip >> next_octet) & 0xff;

    if (curr_octet > ASCII_RANGE) {
      while (curr_octet != 0) {
        tmp_octet[--oct_pos] = (curr_octet % 10) + '0';
        curr_octet /= 10;
      }

      while (tmp_octet[oct_pos] != 0)
        str[str_pos++] = tmp_octet[oct_pos++];

      if (tot_octet != NUM_OCTETS - 1)
        str[str_pos++] = '.';

    } else {
      str[str_pos++] = curr_octet + '0';
      if (tot_octet != NUM_OCTETS - 1)
        str[str_pos++] = '.';
    }
    tot_octet++;
  }
  printf("%s\n", str);
}

/*
  char *ip;
  ip = "0.0.0.1";
  atoip(ip, strlen(ip));
  ip = "1.0.0.1";
  atoip(ip, strlen(ip));
  ip = "10.1.1.1";
  atoip(ip, strlen(ip));
  ip = "10.100.100.100";
  atoip(ip, strlen(ip));

  iptoa(INADDR_LOOPBACK);
  iptoa(INADDR_ANY);
  iptoa(INADDR_BROADCAST);
  iptoa(0x8EFB1E65);
*/
jaso
