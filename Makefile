CC=gcc
BIN=./bin

all : Client Server

Client: ./Client/Client.c
	$(CC) -o $(BIN)/$@ $<

Server: ./Server/Server.c
	$(CC) -o $(BIN)/$@ $<