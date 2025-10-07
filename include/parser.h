#ifndef _Parser_h
#define _Parser_h

//
typedef enum {
  METHOD,
  PATH,
  VERSION,
  HEADER,
} token_t;

typedef enum {
  IDLE,
  STATUS_LINE,
  FIELD_LINE,
  HAS_BODY,
  MORE_BODY,
} state_t;

// ctx exposes the state machine
// this needs to exist in http.h
typedef struct {
  state_t state;
} context_t;

state_t state = IDLE;

token_t *Parse(char *);

#endif
