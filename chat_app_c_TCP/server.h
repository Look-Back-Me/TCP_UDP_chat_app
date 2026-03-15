// server.h
#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "message.h"   // 공통 메시지 정의 포함

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10

// 클라이언트 구조체
typedef struct {
    int socket;
    struct sockaddr_in addr;
} Client;

// 함수 프로토타입
void start_server();
void *handle_client(void *arg);
void broadcast_message(const char *message, int sender_socket);

#endif // SERVER_H