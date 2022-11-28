CC=gcc
BIN=./bin

all : client server

client: ./Client/client2.c
	$(CC) -o $(BIN)/$@ $<

server: ./Serveur/server2.c
	$(CC) -o $(BIN)/$@ $<