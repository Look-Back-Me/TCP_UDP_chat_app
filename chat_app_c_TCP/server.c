// server.c
#include "server.h"
#include "file_transfer.h"

Client clients[MAX_CLIENTS];   // 클라이언트 목록 관리
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// 클라이언트에게 메시지 브로드캐스트
void broadcast_message(const char *message, int sender_socket) 
{
    MessageHeader header;
    header.type = MSG_CHAT;
    header.size = strlen(message);
    header.seq = 0;

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) 
        {
            send(clients[i].socket, &header, sizeof(header), 0);
            send(clients[i].socket, message, header.size, 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// 클라이언트에게 파일 브로드캐스트
void broadcast_file(int sender_socket, const char *filename) 
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) 
        {
            send_file(clients[i].socket, filename);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// 클라이언트 처리 스레드
void *handle_client(void *arg) 
{
    int client_socket = *(int *)arg;
    free(arg);

    MessageHeader header;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(client_socket, &header, sizeof(header), 0)) > 0) 
    {
        if (header.type == MSG_CHAT) 
        {
            if (recv_all(client_socket, buffer, header.size) <= 0) 
                break;
            buffer[header.size] = '\0';
            broadcast_message(buffer, client_socket);

        } 
        else if (header.type == MSG_FILE_START) 
        {
            // 서버는 파일을 받아 저장한 뒤 다른 클라이언트들에게 전달
            receive_file(client_socket, "received_file.dat");
            broadcast_file(client_socket, "received_file.dat");
        }
    }

    // 클라이언트 연결 종료 처리
    close(client_socket);
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket == client_socket) 
        {
            clients[i].socket = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    return NULL;
}

// 서버 시작
void start_server(int port) 
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) 
    {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Bind failed");
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) 
    {
        perror("Listen failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", port);

    while (1) 
    {
        addr_size = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) 
        {
            perror("Accept failed");
            continue;
        }

        pthread_mutex_lock(&clients_mutex);
        int added = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            if (clients[i].socket == 0) 
            {
                clients[i].socket = client_socket;
                clients[i].addr = client_addr;
                added = 1;
                break;
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        if (added) 
        {
            pthread_t tid;
            int *pclient = malloc(sizeof(int));
            *pclient = client_socket;
            pthread_create(&tid, NULL, handle_client, pclient);
            pthread_detach(tid);
        } 
        else 
        {
            printf("Max clients reached. Connection refused.\n");
            close(client_socket);
        }
    }

    close(server_socket);
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    start_server(port);
    return 0;
}