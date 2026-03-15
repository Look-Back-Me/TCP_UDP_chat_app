// file_transfer_UDP.c
#include "file_transfer_UDP.h"

// UDP 단일 패킷 수신
ssize_t recv_packet_udp(int socket, void *buffer, size_t max_len, struct sockaddr_in *src_addr, socklen_t *addr_len) 
{
    return recvfrom(socket, buffer, max_len, 0, (struct sockaddr *)src_addr, addr_len);
}

// 파일 전송
void send_file_udp(int socket, const char *filename, struct sockaddr_in *dest_addr, socklen_t addr_len) 
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) 
    {
        perror("File open failed");
        return;
    }

    char packet[MAX_PACKET_SIZE];
    MessageHeader *header = (MessageHeader *)packet;
    char *body = packet + sizeof(MessageHeader);
    int seq = 0;
    size_t bytes_read;

    // 파일 전송 시작 알림 (헤더만, 바디 없음)
    header->type = MSG_FILE_START;
    header->size = 0;
    header->seq  = seq++;
    sendto(socket, packet, sizeof(MessageHeader), 0, (struct sockaddr *)dest_addr, addr_len);

    // 파일 데이터 전송
    while ((bytes_read = fread(body, 1, CHUNK_SIZE, fp)) > 0) 
    {
        header->type = MSG_FILE_DATA;
        header->size = (int)bytes_read;
        header->seq  = seq++;
        sendto(socket, packet, sizeof(MessageHeader) + bytes_read, 0, (struct sockaddr *)dest_addr, addr_len);
    }

    // 파일 전송 종료 알림 (헤더만, 바디 없음)
    header->type = MSG_FILE_END;
    header->size = 0;
    header->seq  = seq++;
    sendto(socket, packet, sizeof(MessageHeader), 0, (struct sockaddr *)dest_addr, addr_len);

    fclose(fp);
    printf("File '%s' sent successfully (UDP, %d packets).\n", filename, seq);
}

// 파일 수신 - 상태머신 방식
//        이 함수는 외부 루프에서 이미 읽은 패킷(hdr, body)을 받아 처리하므로
//        다른 클라이언트의 패킷과 인터리빙되어도 안전하다.
//
// 반환값:
//   - MSG_FILE_START : 새 파일 오픈, 파일 포인터 반환
//   - MSG_FILE_DATA  : 데이터 기록, 파일 포인터 유지
//   - MSG_FILE_END   : 파일 닫기, NULL 반환 (완료 신호)
//   - 그 외          : fp 그대로 반환
FILE *handle_file_packet(FILE *fp, const char *output_filename, MessageHeader *hdr, const char *body) 
{
    if (hdr->type == MSG_FILE_START) 
    {
        if (fp != NULL) 
        {
            fclose(fp); // 이전 전송이 비정상 종료된 경우 정리
        }
        fp = fopen(output_filename, "wb");
        if (fp == NULL) 
        {
            perror("File open failed");
        }
        printf("File transfer started: '%s'\n", output_filename);
        return fp;

    } 
    else if (hdr->type == MSG_FILE_DATA) 
    {
        if (fp != NULL) 
        {
            fwrite(body, 1, hdr->size, fp);
        }
        return fp;

    } 
    else if (hdr->type == MSG_FILE_END) 
    {
        if (fp != NULL) 
        {
            fclose(fp);
            printf("File '%s' received successfully.\n", output_filename);
        }
        return NULL; // 완료 신호
    }

    return fp;
}