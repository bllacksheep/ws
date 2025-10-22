// adapted from https://github.com/jamesroutley/write-a-hash-table/
#define MAX_HASH_TABLE 20
#define HT_PRIME_1 151
#define HT_PRIME_2 157

typedef struct {
  char *key;
  char *value;
} header_t;

typedef struct {
  int size;
  int count;
  header_t **headers;
} header_table_t;

header_table_t *new_header_table();
static header_t *new_header(const char *, const char *);
static void free_header(header_t *);
void free_header_table(header_table_t *);
static int ht_hash(const char *, const int, const int);
int ht_get_hash(const char*, const int, const int);
