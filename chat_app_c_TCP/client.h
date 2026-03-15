// client.h
#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "message.h"   // 공통 메시지 정의 포함

#define BUFFER_SIZE 4096

// 함수 프로토타입
void start_client();
void *send_messages(void *arg);
void *receive_messages(void *arg);

#endif // CLIENT_H