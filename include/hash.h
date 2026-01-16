#ifndef _HASH_H
#define _HASH 1

#include <stdint.h>
#include <unistd.h>

#define HT_TABLE_SIZE 31u
typedef struct Item tm_item_t;
typedef struct ThreadMap thread_map_t;

thread_map_t *tls_map_get(void);
void tls_map_init(void);
void tls_inc_map(void);
void tls_map_insert(const uint8_t *, const size_t, const uint8_t *);
const uint8_t *tls_map_lookup(const uint8_t *, const size_t);

#endif
