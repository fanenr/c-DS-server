#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#define error(FMT, ...)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s:%d (%s): ", __FUNCTION__, __LINE__, __FILE__);     \
      fprintf (stderr, FMT, ##__VA_ARGS__);                                   \
      fprintf (stderr, "\n");                                                 \
      __builtin_trap ();                                                      \
    }                                                                         \
  while (0)

#endif
