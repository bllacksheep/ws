// adapted from https://github.com/jamesroutley/write-a-hash-table/
#ifndef _HASHT_h
#define _HASHT_h

#include <stdint.h>
#include <sys/types.h>

#define HT_MAX_SIZE 5
#define HT_PRIME_1 151
#define HT_PRIME_2 157

typedef struct {
  uint8_t *key;
  uint8_t *value;
} ht_item;

typedef struct {
  size_t size;
  size_t count;
  ht_item **items;
} ht_hash_table;

ht_hash_table *ht_new();
static ht_item *ht_new_item(const uint8_t *, const uint8_t *);
static void ht_del_item(ht_item *);
void ht_del_hash_table(ht_hash_table *);
static int32_t ht_hash(const uint8_t *, const int32_t, const int32_t);
static int32_t ht_get_hash(const uint8_t *, const int32_t);
void ht_insert(ht_hash_table *, const uint8_t *, const uint8_t *);

#endif
