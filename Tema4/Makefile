CC=gcc
CFLAGS=-I.

client: client.c requests.c helpers.c buffer.c parson.c user.c library.c
	$(CC) -o client *.c -Wall

run: client
	./client

clean:
	rm -f *.o client
