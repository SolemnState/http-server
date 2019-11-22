CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o net.o file.o mime.o

all: server

server: $(OBJS)
	gcc -o $@ $^

net.o: net.c net.h

server.o: server.c net.h

file.o: file.c file.h

mime.o: mime.c mime.h


clean:
	rm -f $(OBJS)
	rm -f server


.PHONY: all, clean