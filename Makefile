MODE = debug
include config.mk

srcs := main.c api.c table.c mongoose.c
objs := $(srcs:%.c=%.o)
libs := -ljansson

.PHONY: all
all: server

.PHONY: run
run: server
	./server

server: $(objs)
	gcc $(LDFLAGS) $(libs) -o $@ $^

$(objs): %.o: %.c
	gcc $(CFLAGS) -c $<

.PHONY: json
json: clean
	bear -- make

.PHONY: clean
clean:
	rm -f *.o main
