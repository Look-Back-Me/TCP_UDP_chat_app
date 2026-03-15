// client.c
#include "client.h"
#include "file_transfer.h"

int sock;
char client_name[BUFFER_SIZE];   // 클라이언트 이름 저장

// 서버로부터 메시지 수신 스레드
void *receive_messages(void *arg) 
{
    MessageHeader header;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(sock, &header, sizeof(header), 0)) > 0) 
    {
        if (header.type == MSG_CHAT) 
        {
            int msg_len = header.size;
            if (recv_all(sock, buffer, msg_len) <= 0) 
                break;
            buffer[msg_len] = '\0';
            printf("%s\n", buffer);   // 서버에서 받은 메시지 그대로 출력
            fflush(stdout);

        } 
        else if (header.type == MSG_FILE_START) 
        {
            printf("File transfer started...\n");
            receive_file(sock, "downloaded_file.dat");

        } 
        else if (header.type == MSG_FILE_END) 
        {
            printf("File transfer finished.\n");
        }
    }

    return NULL;
}

// 사용자 입력 송신 스레드
void *send_messages(void *arg) 
{
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE * 2 + 64];  // 이름 + 메시지 + 여유 공간

    while (1) 
    {
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) 
        {
            buffer[strcspn(buffer, "\n")] = '\0';

            // 종료 명령어 처리
            if (strcmp(buffer, "q") == 0 || strcmp(buffer, "Q") == 0) 
            {
                printf("Disconnecting...\n");
                close(sock);
                exit(0);
            }

            // 파일 전송 명령어 처리
            if (strncmp(buffer, "/sendfile", 9) == 0) {
                char *filename = buffer + 10; // "/sendfile " 다음 문자열을 파일명으로 사용
                if (strlen(filename) > 0) {
                    send_file(sock, filename);
                } else {
                    printf("Usage: /sendfile <filename>\n");
                }
            } 
            else 
            {
                // [name]: 메시지 형식으로 전송
                snprintf(message, sizeof(message), "[%s]: %s", client_name, buffer);

                MessageHeader header;
                header.type = MSG_CHAT;
                header.size = strlen(message);
                header.seq = 0;

                send(sock, &header, sizeof(header), 0);
                send(sock, message, header.size, 0);
            }
        }
    }

    return NULL;
}

// 클라이언트 시작
void start_client(const char *ip, int port, const char *name) 
{
    struct sockaddr_in server_addr;

    strncpy(client_name, name, BUFFER_SIZE - 1);
    client_name[BUFFER_SIZE - 1] = '\0';

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }

    printf("Connected to server %s:%d as [%s]\n", ip, port, client_name);

    pthread_t send_thread, recv_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_create(&send_thread, NULL, send_messages, NULL);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    close(sock);
}

int main(int argc, char *argv[]) 
{
    if (argc < 4) 
    {
        printf("Usage: %s <server_ip> <port> <name>\n", argv[0]);
        exit(1);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *name = argv[3];

    start_client(ip, port, name);
    return 0;
}