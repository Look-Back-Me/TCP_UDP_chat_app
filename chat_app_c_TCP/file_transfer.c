#include "file_transfer.h"

// 안전한 recv 함수: length 바이트를 모두 읽을 때까지 반복
ssize_t recv_all(int socket, void *buffer, size_t length) {
    size_t received = 0;
    char *buf = (char *)buffer;

    while (received < length) {
        ssize_t r = recv(socket, buf + received, length - received, 0);
        if (r <= 0) {
            return r; // 에러 또는 연결 종료
        }
        received += r;
    }
    return received;
}

// 파일 전송 함수
void send_file(int socket, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("File open failed");
        return;
    }

    MessageHeader header;
    char buffer[CHUNK_SIZE];
    int seq = 0;
    size_t bytes_read;

    // 파일 전송 시작 알림
    header.type = MSG_FILE_START;
    header.size = 0;
    header.seq = seq++;
    send(socket, &header, sizeof(header), 0);

    // 파일 데이터 전송
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        header.type = MSG_FILE_DATA;
        header.size = bytes_read;
        header.seq = seq++;
        send(socket, &header, sizeof(header), 0);
        send(socket, buffer, bytes_read, 0);
    }

    // 파일 전송 종료 알림
    header.type = MSG_FILE_END;
    header.size = 0;
    header.seq = seq++;
    send(socket, &header, sizeof(header), 0);

    fclose(fp);
    printf("File '%s' sent successfully.\n", filename);
}

// 파일 수신 함수
void receive_file(int socket, const char *output_filename) {
    FILE *fp = fopen(output_filename, "wb");
    if (fp == NULL) {
        perror("File open failed");
        return;
    }

    MessageHeader header;
    char buffer[CHUNK_SIZE];

    while (recv_all(socket, &header, sizeof(header)) > 0) {
        if (header.type == MSG_FILE_DATA) {
            if (recv_all(socket, buffer, header.size) <= 0) break;
            fwrite(buffer, 1, header.size, fp);
        } else if (header.type == MSG_FILE_END) {
            printf("File '%s' received successfully.\n", output_filename);
            break;
        }
    }

    fclose(fp);
}