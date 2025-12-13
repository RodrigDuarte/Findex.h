CFLAGS=-Wall -Wextra -pedantic -ggdb

.PHONY: ALL
all: example

example: example.c findex.h
	$(CC) $(CFLAGS) -o example example.c