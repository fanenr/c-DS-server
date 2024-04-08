MODE = debug
include config.mk

srcs := main.c api.c table.c mongoose.c
objs := $(srcs:%.c=%.o)
libs := -ljansson

.PHONY: all run
all: server

run: server
	./server

server: $(objs)
	gcc $(LDFLAGS) $(libs) -o $@ $^

$(objs): %.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: clean
clean:
	rm -f *.o main

.PHONY: json
json: clean
	bear -- make
