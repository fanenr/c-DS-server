CSTD   = -std=gnu11
WARN   = -Wall -Wextra -Werror
NOWARN = -Wno-unused-variable -Wno-unused-function

ifeq ($(MODE), debug)
	LTO_LDFLAGS  =
	LTO_CFLAGS   =
	OPT_LEVEL    = -Og

	DBG_LDFLAGS  = -g
	DBG_CFLAGS   = -ggdb3

	ASAN_CFLAGS  = -fsanitize=address
	ASAN_LDFLAGS = -lasan
endif

ifeq ($(MODE), release)
	LTO_LDFLAGS  = -flto
	LTO_CFLAGS   = -flto
	OPT_LEVEL    = -O2

	DBG_LDFLAGS  =
	DBG_CFLAGS   =

	ASAN_CFLAGS  =
	ASAN_LDFLAGS =
endif

CFLAGS  = $(WARN) $(NOWARN) $(OPT_LEVEL) $(DBG_CFLAGS) $(LTO_CFLAGS) \
          $(CSTD) $(ASAN_CFLAGS)

LDFLAGS = $(DBG_LDFLAGS) $(LTO_LDFLAGS) $(ASAN_LDFLAGS)
