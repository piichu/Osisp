CC = gcc
CFLAGS= -Wall -Wno-unused-parameter -Wno-unused-variable

all: lab6 genFile sortIndex run
lab6: lab6.c 
		$(CC) $(CFLAGS) lab6.c -o lab6
genFile: genFile.c 
		$(CC) $(CFLAGS) genFile.c -o genFile
sortIndex: sortIndex.c 
		$(CC) $(CFLAGS) sortIndex.c -o sortIndex
clean:
		rm lab6 genFile sortIndex

run:
	./lab6 128 2 1 outfile.bin 16
	make clean