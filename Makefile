#basic makefile for the shell
all:
	gcc -o shell main.c shell.c util.c error.c

debug:
	gcc -g -o shell main.c shell.c util.c error.c

clean:
	rm *.o shell
