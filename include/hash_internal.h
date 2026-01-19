#ifndef _HASH_INTERNAL_H
#define _HASH_INTERNAL_H 1

#include <stdint.h>
#include <sys/types.h>

#define HT_PRIME_1 31u
#define HT_PRIME_2 37u

static inline uint32_t ht_hash2(const uint8_t *, const size_t, const uint32_t);
static inline uint32_t ht_get_hash(const uint8_t *, const size_t, const size_t);
static inline void tls_map_clear(thread_map_t *);
#endif
