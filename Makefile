all: server client

server: src/server.c
	gcc -o server src/server.c

client: src/client.c
	gcc -o client src/client.c

clean:
	rm -f server client