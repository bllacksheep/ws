// adapted from https://github.com/jamesroutley/write-a-hash-table/
#define HT_MAX_SIZE 5
#define HT_PRIME_1 151
#define HT_PRIME_2 157

typedef struct {
  char *key;
  char *value;
} ht_item;

typedef struct {
  int size;
  int count;
  ht_item **items;
} ht_hash_table;

ht_hash_table *ht_new();
static ht_item *ht_new_item(const char *, const char *);
static void ht_del_item(ht_item *);
void ht_del_hash_table(ht_hash_table *);
static int ht_hash(const char *, const int, const int);
static int ht_get_hash(const char*, const int, const int);
