#ifndef _LOG_H
#define _LOG_H 1
#include <stdarg.h>
#include <stdio.h>

#if LOG_ENABLED
  #warning "LOGGING ENABLED"
  static void logger(const char* file, int line, const char *fn, const char *fmt, ...);

  static void logger(const char* file, int line, const char *fn, const char *fmt, ...) {
      va_list ap;
      va_start(ap, fmt);
      fprintf(stderr, "%s:%d:%s:", file, line, fn);
      vfprintf(stderr, fmt, ap);
      fputc('\n', stderr);
      va_end(ap);
  }
  #define LOG(fmt, ...) logger(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#else
  #define LOG(...) ((void)0)
#endif

#endif
