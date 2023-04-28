CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

# Portul pe care asculta serverul
PORT = 12345

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

all: server subscriber

common.o: common.c

tcp.o: tcp.c

udp.o: udp.c
# Compileaza server.c
server: server.c common.o tcp.o udp.o -lm

# Compileaza subscriber.c
subscriber: subscriber.c common.o

.PHONY: clean run_server run_client

clean:
	rm -rf server subscriber *.o *.dSYM
