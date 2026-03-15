// client_UDP.h
#ifndef CLIENT_UDP_H
#define CLIENT_UDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "file_transfer_UDP.h"
#include "message.h"

void *receive_messages(void *arg);
void *send_messages(void *arg);
void  start_client(const char *ip, int port, const char *name);

#endif // CLIENT_UDP_H