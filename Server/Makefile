CC = gcc -Wall -g

all: clean server loader


loader:
	$(CC) -o loader loader.c global.h 

server:
	$(CC) -o server server.c global.h whiteboard.h whiteboard.c 

.PHONY: clean

clean:
	rm -f server loader
