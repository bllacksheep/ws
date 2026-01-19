#include <stdarg.h>
#include <stdio.h>

#define LOG(fmt, ...) log(__FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
// external linkage
inline void log(const char* file, int line, const char *fn, const char *fmt, ...);



inline void log(const char* file, int line, const char *fn, const char *fmt, ...) {
    // va_list needed to capture ...
    va_list ap;
    // initialize
    va_start(ap, fmt);
    vfprintf(stderr, "%s, %d, %s %s", file, line, fn, fmt, ap);
    // cleanup
    va_end(ap)
}


void main(void) {

    int x = 1;
    LOG("x=%d", x);
}
