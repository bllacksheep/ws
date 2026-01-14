#ifndef _HASH_H
#define _HASH_H 1

#include <stdint.h>
#include <sys/types.h>

#define TABLE_SIZE 31u
#define HT_PRIME_1 31u
#define HT_PRIME_2 37u

typedef struct {
  const uint8_t *key;
  const uint8_t *value;
  uint64_t epoch;
} Item;


typedef struct {
  Item *items[TABLE_SIZE];
  size_t count;
  uint64_t epoch;
} ThreadMap;

static _Thread_local ThreadMap tls_map;
static _Thread_local int tls_inited;

ThreadMap* tls_map_get(void);
void tls_map_insert(const uint8_t*, const size_t, const uint8_t*);
const uint8_t *tls_map_lookup(const uint8_t*, const size_t);
void tls_map_init();
static inline uint32_t ht_hash2(const uint8_t*, const size_t, const uint32_t);
static inline uint32_t ht_get_hash(const uint8_t*, const size_t, const size_t);
static inline void tls_map_clear(ThreadMap*);
#endif
