MODE = debug

include config.mk

libs := -ljansson
srcs := main.c api.c table.c util.c mongoose.c
objs := $(srcs:%.c=%.o)

.PHONY:	all run
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
