#define MAX_SUPPORTED_HEADERS 5

typedef struct {
  char *key;
  char *value;
} header_t;

typedef struct {
  int size;
  int count;
  header_t **headers;
} header_table_t;
