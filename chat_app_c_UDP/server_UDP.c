// server_UDP.c
#include "server_UDP.h"

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// 클라이언트 등록 (중복 방지)
void add_client(int socket, struct sockaddr_in *addr) 
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket != 0 &&
            clients[i].addr.sin_addr.s_addr == addr->sin_addr.s_addr &&
            clients[i].addr.sin_port == addr->sin_port) 
        {
            pthread_mutex_unlock(&clients_mutex);
            return;
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket == 0) 
        {
            clients[i].socket = socket;
            clients[i].addr   = *addr;
            printf("New client registered: %s:%d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

// 채팅 메시지 브로드캐스트
void broadcast_message(const char *message, struct sockaddr_in *sender_addr, socklen_t addr_len) 
{
    int msg_len = (int)strlen(message);

    // 헤더 + 메시지를 하나의 버퍼에 합침
    char packet[sizeof(MessageHeader) + BUFFER_SIZE];
    MessageHeader *header = (MessageHeader *)packet;
    header->type = MSG_CHAT;
    header->size = msg_len;
    header->seq  = 0;
    memcpy(packet + sizeof(MessageHeader), message, msg_len);
    int packet_len = (int)(sizeof(MessageHeader) + msg_len);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket != 0 &&
            !(clients[i].addr.sin_addr.s_addr == sender_addr->sin_addr.s_addr &&
              clients[i].addr.sin_port == sender_addr->sin_port)) 
        {
            sendto(clients[i].socket, packet, packet_len, 0, (struct sockaddr *)&clients[i].addr, addr_len);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// 파일 브로드캐스트
void broadcast_file(int sender_socket __attribute__((unused)), struct sockaddr_in *sender_addr, socklen_t addr_len, const char *filename) 
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket != 0 &&
            !(clients[i].addr.sin_addr.s_addr == sender_addr->sin_addr.s_addr &&
              clients[i].addr.sin_port == sender_addr->sin_port)) 
        {
            send_file_udp(clients[i].socket, filename, &clients[i].addr, addr_len);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// 서버 수신 메인 루프
// 문제 1
// recv_packet_udp로 전체 패킷(헤더+바디)을 한 번에 수신
//
// 문제 2 - receive_file_udp가 루프 내부에서 recvfrom을 점유:
//   파일 수신 중에는 서버가 다른 클라이언트 패킷을 전혀 처리할 수 없었다.
//   → 수정: handle_file_packet으로 외부 루프에서 패킷 단위 처리(상태머신)
void *handle_client(void *arg) 
{
    int server_socket = *(int *)arg;
    free(arg);

    char packet[MAX_PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    FILE *recv_fp = NULL;

    while (1) 
    {
        // 패킷 전체(헤더+바디)를 한 번에 수신
        ssize_t n = recv_packet_udp(server_socket, packet, sizeof(packet), &client_addr, &addr_len);
        if (n <= 0) 
        {
            perror("recvfrom failed");
            break;
        }
        if (n < (ssize_t)sizeof(MessageHeader)) 
        {
            fprintf(stderr, "Packet too small: %zd bytes\n", n);
            continue;
        }

        // 새 클라이언트 등록
        add_client(server_socket, &client_addr);

        MessageHeader *hdr  = (MessageHeader *)packet;
        char *body = packet + sizeof(MessageHeader);

        if (hdr->type == MSG_CHAT) 
        {
            // 바디가 패킷 안에 이미 포함되어 있음 (별도 recvfrom 불필요)
            if (hdr->size <= 0 || hdr->size > (int)(n - sizeof(MessageHeader))) 
            {
                fprintf(stderr, "Invalid chat message size: %d\n", hdr->size);
                continue;
            }
            body[hdr->size] = '\0';
            // printf("Received chat: %s\n", body);
            broadcast_message(body, &client_addr, addr_len);

        } 
        else if (hdr->type == MSG_FILE_START || hdr->type == MSG_FILE_DATA || hdr->type == MSG_FILE_END) 
        {
            // 상태머신으로 파일 패킷 처리
            recv_fp = handle_file_packet(recv_fp, "received_file.dat", hdr, body);

            // 파일 수신 완료 시 브로드캐스트
            if (hdr->type == MSG_FILE_END && recv_fp == NULL) 
            {
                broadcast_file(server_socket, &client_addr, addr_len, "received_file.dat");
            }
        }
    }

    if (recv_fp != NULL) 
    {
        fclose(recv_fp); // 비정상 종료 시 파일 핸들 정리
    }
    return NULL;
}

void start_server(int port) 
{
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    // SO_REUSEADDR: 재시작 시 "Address already in use" 방지
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Bind failed");
        exit(1);
    }

    printf("UDP server listening on port %d...\n", port);

    pthread_t tid;
    int *psock = malloc(sizeof(int));
    *psock = server_socket;
    pthread_create(&tid, NULL, handle_client, psock);
    pthread_join(tid, NULL);

    close(server_socket);
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    start_server(atoi(argv[1]));
    return 0;
}