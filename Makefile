CC=g++
CFLAGS=-Wall -pthread

all: server client

server: server.cpp
	$(CC) $(CFLAGS) -o server server.cpp

client: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp

clean:
	rm -f server client
