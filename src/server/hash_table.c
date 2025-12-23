#include "hash_table.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const uint8_t *key;
  const uint8_t *value;
  uint64_t epoch;
} Header;

#define TABLE_SIZE 31

typedef struct {
  Header *headers[TABLE_SIZE];
  uint64_t epoch;
  size_t count;
} ThreadMap;

_Thread_local ThreadMap tls_map = {.epoch = 1};

static inline void tls_map_clear(ThreadMap *m) {
  // set epoch as uint8_t check memset impact vs compiler branch prediction
  // optimization
  if (m->epoch == 0) {
    memset(m->headers, 0, sizeof(m->headers));
    m->epoch = 1;
  }
  m->epoch++;
  // if (__builtin_expect(m->epoch == 0, 0)) {
  //   memset(m->buckets, 0, sizeof(m->buckets));
  //   m->epoch = 1;
  // }
}

static inline void tls_map_init() {
  for (int i = 0; i < TABLE_SIZE; i++) {
    Header *header = calloc(1, sizeof(*header));
    if (!header) {
      // handle
    }
    header->epoch = 1;
    tls_map.headers[i] = header;
  }
}

// should only works when map version is ahead of items by 1
static void tls_map_insert(ThreadMap *tm, const uint8_t *k, const size_t kl, const uint8_t *v) {
  size_t try = ht_get_hash(k, kl, 0);
  Header *hdr = tm->headers[try];

  size_t i = 1;
  // they're all NULL to start, no check
  // if try<map while loop never executed, else keep looking
  while (tm->epoch == hdr->epoch) {
    try = ht_get_hash(k, kl, i);
    Header *b = tm->headers[try];
    i++;
  }
  hdr->key = k;
  hdr->value = v;
  hdr->epoch++;
  tm->headers[try] = hdr;
  tls_map.count++;
}

static const uint8_t *tls_map_lookup(ThreadMap *tm, const uint8_t *k, const size_t kl) {
  size_t try = ht_get_hash(k, kl, 0);
  Header *hdr = tm->headers[try];

  size_t i = 1;
  while (1) {
    if (hdr->epoch == tm->epoch)
      if (strcmp((const char *)hdr->key, (const char *)k) == 0)
        return hdr->value;
    i++;
    try = ht_get_hash(k, kl, i);
    hdr = tm->headers[try];
  }
}

// E n-1 s[i]**n-1-i
// i = 0
// hash = hash * prime + char 
// exactly equal to above
static size_t ht_hash2(const uint8_t *k, const size_t n, 
			const int32_t p) {
	size_t h = 0;
	for (int i = 0; i < n; i++) {
		h += (h * p) + k[i];
	} 
	return h;
}

// double hashing + linear probing (incremental 'attempt')
static size_t ht_get_hash(const uint8_t *item_key, const size_t item_key_len, 
		const int32_t attempt) {
  const size_t hash_a = ht_hash2(item_key, item_key_len, HT_PRIME_1);
  const size_t hash_b = ht_hash2(item_key, item_key_len, HT_PRIME_2);
  return (hash_a + (attempt * (hash_b + 1))) % TABLE_SIZE;
}

int main() {
  tls_map_init();
  // new request
  tls_map.epoch++;

  const uint8_t *host_header = "Host";
  const size_t host_header_len = strlen(host_header);
  const uint8_t *upgrade_header = "Upgrade";
  const size_t upgrade_header_len = strlen(upgrade_header);
  const uint8_t *hos_header = "Hos";
  const size_t hos_header_len = strlen(hos_header);
  const uint8_t *connection_header = "Connection";
  const size_t connection_header_len = strlen(connection_header);

  tls_map_insert(&tls_map, host_header, host_header_len, (const uint8_t *)"example.com");
  tls_map_insert(&tls_map, upgrade_header, upgrade_header_len, (const uint8_t *)"sure");
  tls_map_insert(&tls_map, hos_header, hos_header_len, (const uint8_t *)"fake");
  tls_map_insert(&tls_map, connection_header, connection_header_len, (const uint8_t *)"keep-alive");

  const uint8_t *v1 = tls_map_lookup(&tls_map, host_header, host_header_len);
  printf("Host is: %s\n", v1);
  const uint8_t *v2 = tls_map_lookup(&tls_map, upgrade_header, upgrade_header_len);
  printf("Upgrade is: %s\n", v2);
  const uint8_t *v3 = tls_map_lookup(&tls_map, hos_header, hos_header_len);
  printf("Hos is: %s\n", v3);
  const uint8_t *v4 = tls_map_lookup(&tls_map, connection_header, connection_header_len);
  printf("Connection is: %s\n", v4);

  // new request
  tls_map.epoch++;
  tls_map_insert(&tls_map, host_header, host_header_len, (const uint8_t *)"new.com");
  tls_map_insert(&tls_map, upgrade_header, upgrade_header_len, (const uint8_t *)"no");
  tls_map_insert(&tls_map, hos_header, hos_header_len, (const uint8_t *)"real");
  tls_map_insert(&tls_map, connection_header, connection_header_len, (const uint8_t *)"close");

  const uint8_t *v5 = tls_map_lookup(&tls_map, host_header, host_header_len);
  printf("Host is: %s\n", v5);
  const uint8_t *v6 = tls_map_lookup(&tls_map, upgrade_header, upgrade_header_len);
  printf("Upgrade is: %s\n", v6);
  const uint8_t *v7 = tls_map_lookup(&tls_map, hos_header, hos_header_len);
  printf("Hos is: %s\n", v7);
  const uint8_t *v8 = tls_map_lookup(&tls_map, connection_header, connection_header_len);
  printf("Connection is: %s\n", v8);
}
