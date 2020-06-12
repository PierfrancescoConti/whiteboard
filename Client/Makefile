CC = gcc -Wall -g

all: clean client test test_BOF


test:
	$(CC) -o test test.c global.h


test_BOF:
	$(CC) -o test_BOF test_BOF.c global.h 


client:
	$(CC) -o client client.c global.h 


.PHONY: clean

clean:
	rm -f client test test_BOF
