#include "hash_table.h"
#include <stdlib.h>
#include <string.h>

static header_t *new_header(const char *k, const char *v) {
  header_t *hdr = malloc(sizeof(header_t));
  hdr->key = strdup(k);
  hdr->value = strdup(v);
  return hdr;
}

header_table_t *new_header_table() {
  header_table_t *hdrs = malloc(sizeof(header_table_t));

  hdrs->size = MAX_SUPPORTED_HEADERS;
  hdrs->count = 0;
  hdrs->headers = calloc((size_t)hdrs->size, sizeof(header_t *));

  return hdrs;
}
