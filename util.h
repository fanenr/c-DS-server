#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define error(FMT, ...)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s (%s:%d): ", __FILE__, __FUNCTION__, __LINE__);     \
      fprintf (stderr, FMT, ##__VA_ARGS__);                                   \
      fprintf (stderr, "\n");                                                 \
      quit ();                                                                \
    }                                                                         \
  while (0)

extern void quit (void) __attribute__ ((noreturn));

#endif
