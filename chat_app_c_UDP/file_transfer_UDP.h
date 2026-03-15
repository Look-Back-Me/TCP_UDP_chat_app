// file_transfer_UDP.h
#ifndef FILE_TRANSFER_UDP_H
#define FILE_TRANSFER_UDP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "message.h"

// MTU(1500) - IP헤더(20) - UDP헤더(8) = 1472. 여유분 제외하여 1400으로 설정
// 기존 65536은 IP 단편화를 유발하고, 단편 하나라도 손실 시 전체 패킷 폐기됨
#define CHUNK_SIZE 1400

// 최대 패킷 크기: 헤더 + 데이터
#define MAX_PACKET_SIZE (sizeof(MessageHeader) + CHUNK_SIZE)

// 버퍼 크기
#define BUFFER_SIZE 2048

// UDP 단일 패킷 수신 (recvfrom 1회 호출)
// - UDP는 recvfrom 한 번으로 패킷 전체를 수신한다.
ssize_t recv_packet_udp(int socket, void *buffer, size_t max_len, struct sockaddr_in *src_addr, socklen_t *addr_len);

// 파일 전송 (헤더+데이터 묶어서 단일 sendto)
void send_file_udp(int socket, const char *filename, struct sockaddr_in *dest_addr, socklen_t addr_len);

// 파일 수신
// fp가 NULL이면 새 파일을 열고, MSG_FILE_END 수신 시 파일을 닫고 NULL 반환
FILE *handle_file_packet(FILE *fp, const char *output_filename, MessageHeader *hdr, const char *body);

#endif // FILE_TRANSFER_UDP_H