#ifndef _HASH_H
#define _HASH_H 1

#include <stdint.h>
#include <sys/types.h>

#define TABLE_SIZE 31u
#define HT_PRIME_1 31u
#define HT_PRIME_2 37u

typedef struct Item tm_item_t;
typedef struct ThreadMap thread_map_t;
static _Thread_local thread_map_t tls_map;
static _Thread_local int tls_inited;

thread_map_t* tls_map_get(void);
void tls_map_init(void);
void tls_map_insert(const uint8_t*, const size_t, const uint8_t*);
const uint8_t *tls_map_lookup(const uint8_t*, const size_t);
static inline uint32_t ht_hash2(const uint8_t*, const size_t, const uint32_t);
static inline uint32_t ht_get_hash(const uint8_t*, const size_t, const size_t);
static inline void tls_map_clear(thread_map_t*);
#endif
