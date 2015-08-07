
VPATH = ../include ../src ../test
CFLAGS = -g --std=c11

objs = threadpool_func.o printarray.o

all: printarray

printarray: $(objs)
	$(CC) $(CFLAGS) -I../include $^ -o $@ -pthread

threadpool_func.o: threadpool_func.c threadpool.h
	$(CC) $(CFLAGS) -I../include -c $< -o $@ 

%.o: %.c threadpool.h
	$(CC) $(CFLAGS) -I../include -c $< -o $@

.PHONY: clean
	rm $(objs)
