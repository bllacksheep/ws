#include "hash_internal.h"
#include "hash.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Item {
  const uint8_t *key;
  const uint8_t *value;
  uint64_t epoch;
} tm_item_t;

typedef struct ThreadMap {
  tm_item_t *items[HT_TABLE_SIZE];
  size_t count;
  uint64_t epoch;
} thread_map_t;

static _Thread_local thread_map_t tls_map;
static _Thread_local int tls_inited;

thread_map_t* tls_map_get(void) {
	if (!tls_inited) tls_map_init();
	return &tls_map;
}

void tls_inc_map(void) {
    thread_map_t *tm = tls_map_get();
    tm->epoch++;
}

static inline void tls_map_clear(thread_map_t *tm) {
  // pre-condition checks in caller
  memset(tm->items, 0, sizeof(tm->items));
  tm->epoch = 1;
}

void tls_map_init(void) {
  if (tls_inited) return;

  tls_map.epoch = 1;
  for (int i = 0; i < HT_TABLE_SIZE; i++) {
    tm_item_t *item = calloc(1, sizeof(*item));
    if (item == NULL) {
        fprintf(stderr, "could not initialize tls_map\n");
        exit(-1);
    }
    item->epoch = 1;
    tls_map.items[i] = item;
  }
  tls_inited = 1;
}

// should only works when map version is ahead of items by 1
void tls_map_insert(const uint8_t *k, const size_t kl, const uint8_t *v) {

  thread_map_t* tm = tls_map_get();

  uint32_t try = ht_get_hash(k, kl, 0u);
  tm_item_t *item = tm->items[try];

  size_t i = 1;
  // they're all NULL to start, no check
  // if try<map while loop never executed, else keep looking
  while (tm->epoch == item->epoch) {
    try = ht_get_hash(k, kl, i);
    tm_item_t *b = tm->items[try];
    i++;
  }
  item->key = k;
  item->value = v;
  item->epoch++;
  tm->items[try] = item;
  tls_map.count++;
}

const uint8_t *tls_map_lookup(const uint8_t *k, const size_t kl) {

  thread_map_t *tm = tls_map_get();

  uint32_t try = ht_get_hash(k, kl, 0);
  tm_item_t *item = tm->items[try];

  size_t i = 1;
  while (1) {
    if (item->epoch == tm->epoch)
      if (strcmp((const char *)item->key, (const char *)k) == 0)
        return item->value;
    i++;
    try = ht_get_hash(k, kl, i);
    item = tm->items[try];
  }
}

// E n-1 s[i]**n-1-i
// i = 0
// hash = hash * prime + char 
// exactly equal to above
static inline uint32_t ht_hash2(const uint8_t *k, const size_t n, 
			const uint32_t p) {
	uint64_t h = 0;
	for (size_t i = 0; i < n; i++) {
		h += (h * p) + k[i];
	} 
	return (uint32_t)h % p;
}

// double hashing + linear probing (incremental 'attempt')
static inline uint32_t ht_get_hash(const uint8_t *item_key, const size_t item_key_len, 
		const size_t attempt) {
  const uint32_t hash_a = ht_hash2(item_key, item_key_len, HT_PRIME_1);
  const uint32_t hash_b = ht_hash2(item_key, item_key_len, HT_PRIME_2);
  // should these types align in shape?
  return (hash_a + (attempt * (hash_b + 1))) % HT_TABLE_SIZE;
}
/*
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
*/
