# 컴파일러와 옵션
CC = gcc
CFLAGS = -Wall -pthread

# 대상 실행 파일
TARGETS = server client

# 소스 파일
SERVER_SRC = server.c file_transfer.c
CLIENT_SRC = client.c file_transfer.c

# 기본 빌드
all: $(TARGETS)

# 서버 빌드
server: $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC)

# 클라이언트 빌드
client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_SRC)

# 청소(clean)
clean:
	rm -f $(TARGETS) *.o *.dat