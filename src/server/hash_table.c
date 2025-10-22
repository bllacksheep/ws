#include "hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static header_t *new_header(const char *k, const char *v) {
  header_t *hdr = malloc(sizeof(header_t));
  hdr->key = strdup(k);
  hdr->value = strdup(v);
  return hdr;
}

header_table_t *new_header_table() {
  header_table_t *header_table = malloc(sizeof(header_table_t));

  header_table->size = MAX_HASH_TABLE;
  header_table->count = 0;
  header_table->headers =
      calloc((size_t)header_table->size, sizeof(header_t *));

  return header_table;
}

static void free_header(header_t *h) {
  free(h->key);
  free(h->value);
  free(h);
}

void free_header_table(header_table_t *header_table) {
  for (int i = 0; i < header_table->size; i++) {
    header_t *header = header_table->headers[i];
    if (header != NULL) {
      free_header(header);
    }
  }
  free(header_table->headers);
  free(header_table);
}

static int ht_hash(const char *s, const int a, const int m) {
    long hash = 0;
    const int len_s = strlen(s);
    for (int i = 0; i < len_s; i++) {
        hash += (long)pow(a, len_s - (i+1)) * s[i];
        hash = hash % m;
    }
    return (int)hash;
}

int ht_get_hash(const char* s, const int num_buckets, const int attempt) {
    const int hash_a = ht_hash(s, HT_PRIME_1, num_buckets);
    const int hash_b = ht_hash(s, HT_PRIME_2, num_buckets);
    return (hash_a + (attempt * (hash_b + 1))) % num_buckets;
}




