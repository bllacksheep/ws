#include "hash_table.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint8_t *key;
  uint8_t *value;
  uint64_t epoch;
} Bucket;

#define BUCKET_COUNT 32

typedef struct {
  Bucket buckets[BUCKET_COUNT];
  uint8_t epoch;
  size_t size;
} ThreadMap;

_Thread_local ThreadMap tls_map = {.epoch = 1};

static inline void map_clear(ThreadMap *m) {
  m->epoch++;
  // set epoch as uint8_t check memset impact vs compiler branch prediction
  // optimization
  if (m->epoch == 0) {
    memset(m->buckets, 0, sizeof(m->buckets));
    m->epoch = 1;
  }
  // if (__builtin_expect(m->epoch == 0, 0)) {
  //   memset(m->buckets, 0, sizeof(m->buckets));
  //   m->epoch = 1;
  // }
  m->size = 0;
}

static ht_item *ht_new_item(const uint8_t *k, const uint8_t *v) {
  ht_item *i = malloc(sizeof(ht_item));
  // printf("ht item alloc %zuB\n", sizeof(ht_item));
  i->key = (uint8_t *)strdup((const char *)k);
  // printf("item k alloc %zuB\n", sizeof(uint8_t) * strlen((const char *)k));
  i->value = (uint8_t *)strdup((const char *)v);
  // printf("item v alloc %zuB\n", sizeof(uint8_t) * strlen((const char *)v));

  return i;
}

ht_hash_table *ht_new() {
  ht_hash_table *ht = malloc(sizeof(ht_hash_table));
  // printf("ht alloc %zuB\n", sizeof(ht_hash_table));

  ht->size = HT_MAX_SIZE;
  ht->count = 0;
  ht->items = calloc((size_t)ht->size, sizeof(ht_item *));

  // printf("ht items alloc %zuB\n", sizeof(ht_item) * ht->size);

  return ht;
}

static void ht_del_item(ht_item *i) {
  free(i->key);
  free(i->value);
  free(i);
}

void ht_del_hash_table(ht_hash_table *ht) {
  for (int i = 0; i < ht->size; i++) {
    ht_item *item = ht->items[i];
    if (item != NULL) {
      ht_del_item(item);
    }
  }
  free(ht->items);
  free(ht);
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

static int32_t ht_get_hash(const uint8_t *s, const int32_t num_buckets,
                           const int32_t attempt) {
  const int32_t hash_a = ht_hash(s, HT_PRIME_1, num_buckets);
  const int32_t hash_b = ht_hash(s, HT_PRIME_2, num_buckets);
  return (hash_a + (attempt * (hash_b + 1))) % num_buckets;
}

void ht_insert(ht_hash_table *ht, const uint8_t *key, const uint8_t *value) {
  ht_item *item = ht_new_item(key, value);
  int32_t index = ht_get_hash(item->key, ht->size, 0);
  ht_item *cur_item = ht->items[index];
  int32_t i = 1;
  while (cur_item != NULL) {
    index = ht_get_hash(item->key, ht->size, i);
    cur_item = ht->items[index];
    i++;
  }
  ht->items[index] = item;
  ht->count++;
  // printf("k: %s, i: %d, v: %s\n", key, index, value);
}
