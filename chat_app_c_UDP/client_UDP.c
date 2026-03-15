// client_UDP.c
#include "client_UDP.h"
#include "file_transfer_UDP.h"

static int                sock;
static char               client_name[BUFFER_SIZE];
static struct sockaddr_in server_addr;       // 서버 주소
static socklen_t          server_addr_len = sizeof(server_addr);

// 서버로부터 메시지 수신 스레드
// [수정 1] recv_all_udp 루프 → recv_packet_udp 단일 호출
//          기존 코드는 헤더를 recvfrom으로 읽은 뒤 바디를 다시 recvfrom으로
//          읽었는데, 이 사이에 다른 패킷이 도착하면 바디 자리에 잘못 읽힌다.
//          → 패킷 전체(헤더+바디)를 한 번에 수신
//
// [수정 2] server_addr 덮어쓰기 방지
//          기존 코드는 recv_all_udp의 src_addr로 &server_addr를 직접 넘겼다.
//          recvfrom은 실제 송신자 주소로 src_addr를 덮어쓰므로, 저장된
//          서버 주소가 변경되어 이후 sendto가 엉뚱한 곳으로 전송될 수 있었다.
//          → 임시 from_addr를 사용해 server_addr를 보호
void *receive_messages(void *arg __attribute__((unused))) 
{
    char packet[MAX_PACKET_SIZE];
    FILE *recv_fp = NULL;

    while (1) 
    {
        // server_addr 대신 임시 주소로 수신
        struct sockaddr_in from_addr;
        socklen_t from_len = sizeof(from_addr);

        ssize_t n = recv_packet_udp(sock, packet, sizeof(packet), &from_addr, &from_len);
        if (n <= 0) 
            break;
        if (n < (ssize_t)sizeof(MessageHeader)) 
            continue;

        MessageHeader *hdr  = (MessageHeader *)packet;
        char *body = packet + sizeof(MessageHeader);

        if (hdr->type == MSG_CHAT) 
        {
            if (hdr->size <= 0 || hdr->size > (int)(n - sizeof(MessageHeader))) 
                continue;
            body[hdr->size] = '\0';
            printf("%s\n", body);
            fflush(stdout);

        } 
        else if (hdr->type == MSG_FILE_START || hdr->type == MSG_FILE_DATA  || hdr->type == MSG_FILE_END) 
        {
            recv_fp = handle_file_packet(recv_fp, "downloaded_file.dat", hdr, body);
        }
    }

    if (recv_fp != NULL) fclose(recv_fp);
    return NULL;
}

// 사용자 입력 송신 스레드
void *send_messages(void *arg __attribute__((unused))) 
{
    char input[BUFFER_SIZE];
    char message[BUFFER_SIZE * 2 + 64];

    while (1) {
        if (fgets(input, BUFFER_SIZE, stdin) == NULL) break;
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) 
        {
            printf("Disconnecting...\n");
            close(sock);
            exit(0);
        }

        if (strncmp(input, "/sendfile ", 10) == 0) 
        {
            const char *filename = input + 10;
            if (strlen(filename) > 0) 
            {
                send_file_udp(sock, filename, &server_addr, server_addr_len);
            } 
            else 
            {
                printf("Usage: /sendfile <filename>\n");
            }
        } 
        else 
        {
            snprintf(message, sizeof(message), "[%s]: %s", client_name, input);

            // 헤더+바디를 하나의 패킷으로 묶어 전송
            int msg_len = (int)strlen(message);
            char packet[sizeof(MessageHeader) + BUFFER_SIZE * 2 + 64];
            MessageHeader *header = (MessageHeader *)packet;
            header->type = MSG_CHAT;
            header->size = msg_len;
            header->seq  = 0;
            memcpy(packet + sizeof(MessageHeader), message, msg_len);

            sendto(sock, packet, sizeof(MessageHeader) + msg_len, 0, (struct sockaddr *)&server_addr, server_addr_len);
        }
    }
    return NULL;
}

void start_client(const char *ip, int port, const char *name) 
{
    strncpy(client_name, name, BUFFER_SIZE - 1);
    client_name[BUFFER_SIZE - 1] = '\0';

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    printf("UDP client ready for server %s:%d as [%s]\n", ip, port, client_name);
    printf("Commands: /sendfile <filename> | q to quit\n");

    pthread_t send_thread, recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_create(&send_thread, NULL, send_messages,    NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(sock);
}

int main(int argc, char *argv[]) 
{
    if (argc < 4) {
        printf("Usage: %s <server_ip> <port> <name>\n", argv[0]);
        exit(1);
    }
    start_client(argv[1], atoi(argv[2]), argv[3]);
    return 0;
}