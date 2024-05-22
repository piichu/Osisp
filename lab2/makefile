CC = gcc
CFLAGS_DEBUG = -g -ggdb -std=c11 -pedantic -W -Wall -Wextra
CFLAGS_RELEASE = -std=c11 -pedantic -W -Wall -Wextra
.PHONY: clean all run
MODE ?= debug
ifeq ($(MODE), debug)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif
all: main child
main: main.c makefile
	$(CC) $(CFLAGS) main.c -o main
child: child.c makefile
	$(CC) $(CFLAGS) child.c -o child
run:
	./main args.txt
clean:
	rm -f child
	rm -f main
	rm -rf main.dSYM
	rm -rf child.dSYM