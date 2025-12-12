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

#define BUCKET_COUNT 32

typedef struct {
  Header *headers[BUCKET_COUNT];
  uint64_t epoch;
  size_t count;
} ThreadMap;

_Thread_local ThreadMap tls_map = {.epoch = 1};

static inline void map_clear(ThreadMap *m) {
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
  for (int i = 0; i < BUCKET_COUNT; i++) {
    Header *header = calloc(sizeof(Header), 0);
    header->epoch = 1;
    tls_map.headers[i] = header;
  }
}

// only works if map version is a head per request
static void tls_map_insert(ThreadMap *tm, const uint8_t *k, const uint8_t *v) {
  int32_t try = ht_get_hash(k, 0);
  Header *hdr = tm->headers[try];

  int32_t i = 1;
  // they're all NULL to start, no check
  // if try<map while loop never executed, else keep looking
  while (tm->epoch == hdr->epoch) {
    try = ht_get_hash(k, i);
    Header *b = tm->headers[try];
    i++;
  }
  hdr->key = k;
  hdr->value = v;
  hdr->epoch++;
  tm->headers[try] = hdr;
  tls_map.count++;
}

static const uint8_t *tls_map_lookup(ThreadMap *tm, const uint8_t *k) {
  int32_t try = ht_get_hash(k, 0);
  Header *hdr = tm->headers[try];

  int32_t i = 1;
  while (1) {
    if (strcmp((const char *)hdr->key, (const char *)k) == 0) {
      return hdr->value;
    }
    i++;
    try = ht_get_hash(k, i);
    hdr = tm->headers[try];
  }
}

static int32_t ht_hash(const uint8_t *s, const int32_t a, const int32_t m) {
  int64_t hash = 0;
  const int32_t len_s = strlen((const char *)s);
  for (int32_t i = 0; i < len_s; i++) {
    hash += (int64_t)pow(a, len_s - (i + 1)) * s[i];
    hash = hash % m;
  }
  return (int32_t)hash;
}

static int32_t ht_get_hash(const uint8_t *s, const int32_t attempt) {
  const int32_t hash_a = ht_hash(s, HT_PRIME_1, BUCKET_COUNT);
  const int32_t hash_b = ht_hash(s, HT_PRIME_2, BUCKET_COUNT);
  return (hash_a + (attempt * (hash_b + 1))) % BUCKET_COUNT;
}

int main() {
  tls_map_init();
  tls_map.epoch++; // assume new request
  tls_map_insert(&tls_map, (const uint8_t *)"Host", (const uint8_t *)"12324");
  const uint8_t *v = tls_map_lookup(&tls_map, (const uint8_t *)"Host");
  printf("val: %s\n", v);
}
