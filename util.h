#ifndef UTIL_H
#define UTIL_H

#define error(FMT, ...)                                                       \
  eprintf ("%s (%d): " FMT "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

extern void eprintf (const char *fmt, ...)
    __attribute__ ((noreturn, format (printf, 1, 2)));

#endif
