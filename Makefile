all: 
	gcc server.c -lsqlite3 -std=c99 -o server
	gcc client.c -o client

