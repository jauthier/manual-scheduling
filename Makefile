CFLAGS=-Wall -Wextra -Werror -g
CC=clang

all: example

example: example.o lib.o
	$(CC) $^ -o $@

clean:
	-rm example example.o lib.o
