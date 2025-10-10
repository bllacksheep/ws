#ifndef _Parser_h
#define _Parser_h

//
typedef enum {
  T_METHOD,
  T_PATH,
  T_VERSION,
  T_HEADER,
  T_SPACE,
  T_SEPARATOR,
  T_TERMINATOR,
} token_t;

typedef enum {
  S_IDLE,
  S_STATUS_LINE,
  S_FIELD_LINE,
  S_HAS_BODY,
  S_MORE_BODY,
} state_t;

// ctx exposes the state machine
// this needs to exist in http.h
typedef struct {
  state_t state;
} context_t;

state_t state = S_IDLE;

void Parse(char *, token_t *);

#endif
