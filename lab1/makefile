# makefile
CC = gcc
CFLAGS = -W -Wall -Wextra -std=c11 -D_BSD_SOURCE
.PHONY: clean
all: prog
prog: main.c makefile
	$(CC) $(CFLAGS) main.c -o prog
clean: first second
first:
	rm prog
second:
#	rm -r .vscode