CC     = gcc
CFLAGS = -Wall -Wextra -pthread

all: server client

server: server_UDP.c file_transfer_UDP.c
	$(CC) $(CFLAGS) -o server_UDP server_UDP.c file_transfer_UDP.c

client: client_UDP.c file_transfer_UDP.c
	$(CC) $(CFLAGS) -o client_UDP client_UDP.c file_transfer_UDP.c

clean:
	rm -f server_UDP client_UDP received_file.dat downloaded_file.dat