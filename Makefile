CC = gcc -Wall -g

all: clean client server test test_BOF


test:
	$(CC) -o test test.c global.h 


test_BOF:
	$(CC) -o test_BOF test_BOF.c global.h 


client:
	$(CC) -o client client.c global.h 


server:
	$(CC) -o server server.c global.h whiteboard.h whiteboard.c 

.PHONY: clean

clean:
	rm -f client server test test_BOF
