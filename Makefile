CC=gcc
BIN=./bin

all : client server

client: ./Client/Client.c
	$(CC) -o $(BIN)/$@ $<

server: ./Server/Server.c
	$(CC) -o $(BIN)/$@ $<