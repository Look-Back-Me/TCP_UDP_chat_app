// server_UDP.h
#ifndef SERVER_UDP_H
#define SERVER_UDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "file_transfer_UDP.h"
#include "message.h"

#define MAX_CLIENTS 32

typedef struct {
    int    socket;
    struct sockaddr_in addr;
} Client;

void add_client(int socket, struct sockaddr_in *addr);
void broadcast_message(const char *message, struct sockaddr_in *sender_addr, socklen_t addr_len);
void broadcast_file(int sender_socket, struct sockaddr_in *sender_addr, socklen_t addr_len, const char *filename);
void *handle_client(void *arg);
void  start_server(int port);

#endif // SERVER_UDP_H