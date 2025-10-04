#ifndef _Parser_h
#define _Parser_h

typedef enum {
  METHOD,
  PATH,
  VERSION,
  HEADER,
} token_t;

typedef enum {
  STATUS_LINE,
  FIELD_LINE,
  HAS_BODY,
  MORE_BODY,
} state_t;

typedef struct {
  state_t state;
} context_t;

token_t *Parse(char *);

#endif
