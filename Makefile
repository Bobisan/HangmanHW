
CC = gcc
CFLAGS = -Wall -Wextra -g
 
all: hangman-server hangman-client
 
hangman-server: src/server.c src/game.c include/game.h
	$(CC) $(CFLAGS) -o hangman-server src/server.c
 
hangman-client: src/client.c src/game.c include/game.h
	$(CC) $(CFLAGS) -o hangman-client src/client.c
 
clean:
	rm -f hangman-server hangman-client
 
.PHONY: all clean