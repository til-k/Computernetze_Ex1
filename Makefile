SHELL = /bin/sh
.SUFFIXES:
.SUFFIXES: .c .o

CC = gcc
CFLAGS = -Wall

BIN = bin
BUILD = build

SRC = src
CLIENT_SRC_PATH = $(SRC)/client
SERVER_SRC_PATH = $(SRC)/server
CLIENT_EXE_NAME = client
SERVER_EXE_NAME = server

all: client server

client: 
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(CLIENT_SRC_PATH)/*.c -o $(BIN)/$(CLIENT_EXE_NAME)

server: 
	mkdir -p $(BIN)
	$(CC) $(CFLAGS) $(SERVER_SRC_PATH)/*.c -o $(BIN)/$(SERVER_EXE_NAME)

clean: 
	rm -f $(BIN)/$(CLIENT_EXE_NAME)
	rm -f $(BIN)/$(SERVER_EXE_NAME)
	rm -rf *o $(BUILD)/*
	echo Cleaned build and bin folders
