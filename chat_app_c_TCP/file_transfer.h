// file_transfer.h
#ifndef FILE_TRANSFER_H
#define FILE_TRANSFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"   

// 대용량 파일 전송 최적화
#define CHUNK_SIZE 65536  

// 파일 전송 함수 프로토타입
void send_file(int socket, const char *filename);
void receive_file(int socket, const char *output_filename);
ssize_t recv_all(int socket, void *buffer, size_t length);

#endif // FILE_TRANSFER_H