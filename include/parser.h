#ifndef _Parser_h
#define _Parser_h

typedef enum tokens_t {
  METHOD,
  PATH,
  VERSION,
  HEADER,
} tokens_t;

tokens_t *Parse(char *);

#endif
