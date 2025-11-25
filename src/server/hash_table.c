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
} Bucket;

#define BUCKET_COUNT 32

typedef struct {
  Bucket *buckets[BUCKET_COUNT];
  uint64_t epoch;
  size_t size;
} ThreadMap;

_Thread_local ThreadMap tls_map = {.epoch = 0, .size = BUCKET_COUNT};

static inline void map_clear(ThreadMap *m) {
  // set epoch as uint8_t check memset impact vs compiler branch prediction
  // optimization
  if (m->epoch == 0) {
    memset(m->buckets, 0, sizeof(m->buckets));
    m->epoch = 1;
  }
  m->epoch++;
  // if (__builtin_expect(m->epoch == 0, 0)) {
  //   memset(m->buckets, 0, sizeof(m->buckets));
  //   m->epoch = 1;
  // }
}

static inline void new_map() {
  for (int i = 0; i < BUCKET_COUNT; i++) {
    Bucket *bucket = malloc(sizeof(Bucket));
    bucket->key = NULL;
    bucket->value = NULL;
    bucket->epoch = 1;
    tls_map.buckets[i] = bucket;
  }
}

static void insert(ThreadMap *m, const uint8_t *k, const uint8_t *v) {
  int32_t index = ht_get_hash(k, 0);
  Bucket *b = m->buckets[index];

  int32_t i = 1;
  while (b != NULL && m->epoch == b->epoch) {
    index = ht_get_hash(k, i);
    Bucket *b = m->buckets[index];
    i++;
  }
  b->key = k;
  b->value = v;
  b->epoch++;
  m->buckets[index] = b;
}

static const uint8_t *get_map(ThreadMap *m, const uint8_t *k) {
  int32_t index = ht_get_hash(k, 0);
  Bucket *b = m->buckets[index];

  int32_t i = 1;
  while (1) {
    if (strcmp((const char *)b->key, (const char *)k) == 0) {
      return b->value;
    }
    i++;
    index = ht_get_hash(k, i);
    Bucket *b = m->buckets[index];
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
  new_map();
  insert(&tls_map, (const uint8_t *)"Host", (const uint8_t *)"12324");
  const uint8_t *v = get_map(&tls_map, (const uint8_t *)"Host");
  printf("val: %s\n", v);
}
