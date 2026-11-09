CC = gcc
CFLAGS = -Wall

all: server client awale

server: server2.c server2.h jeu.c jeu.h
	$(CC) $(CFLAGS) -o server server2.c jeu.c

client: client2.c client2.h
	$(CC) $(CFLAGS) -o client client2.c

awale: awale.c jeu.c jeu.h
	$(CC) $(CFLAGS) -o awale awale.c jeu.c

clean:
	rm -f server client awale
