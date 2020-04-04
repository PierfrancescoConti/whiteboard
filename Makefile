CC = gcc -Wall -g

all: clean client server



client:
	$(CC) -o client client.c global.h



server:
	$(CC) -o server server.c global.h whiteboard.h whiteboard.c

.PHONY: clean

clean:
	rm -f client server
