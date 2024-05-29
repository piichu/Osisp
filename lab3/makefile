CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic -Werror -ggdb

all: parent child

parent: parent.c makefile
	$(CC) $(CFLAGS) parent.c -o parent

child: child.c makefile
	$(CC) $(CFLAGS) child.c -o child

.PHONY: clean

clean:
	rm -f parent child

run:
	./parent