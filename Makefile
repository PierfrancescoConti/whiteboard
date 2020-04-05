CC = gcc -Wall -g

all: clean client server



client:
	$(CC) -o client client.c global.h -Wno-unused-variable



server:
	$(CC) -o server server.c global.h whiteboard.h whiteboard.c -Wno-unused-variable

.PHONY: clean

clean:
	rm -f client server
