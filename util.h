#ifndef UTIL_H
#define UTIL_H

/*
 * 遇到致命错误并退出
 */

#define error(FMT, ...) eprintf ("%s: " FMT "\n", __FUNCTION__, ##__VA_ARGS__)

extern void eprintf (const char *fmt, ...)
    __attribute__ ((noreturn, format (printf, 1, 2)));

#endif
