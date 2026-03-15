import struct
import socket

CHUNK_SIZE = 1400

# MessageHeader: type(int), size(int), seq(int) — C 구조체와 동일
HEADER_FORMAT = "<iii"   # little-endian, TCP 버전의 "iii"와 바이트 호환
HEADER_SIZE   = struct.calcsize(HEADER_FORMAT)  # 12바이트

MSG_CHAT       = 1
MSG_FILE_START = 2
MSG_FILE_DATA  = 3
MSG_FILE_END   = 4

MAX_BUF = HEADER_SIZE + CHUNK_SIZE + 64  # recvfrom 버퍼 크기


# 패킷 함수 (UDP는 헤더+바디를 한 번에 보내고 받는다) 

def make_packet(msg_type: int, body: bytes = b"", seq: int = 0):
    """헤더 + 바디를 하나의 UDP 패킷으로 만든다."""
    header = struct.pack(HEADER_FORMAT, msg_type, len(body), seq)
    return header + body


def parse_packet(data: bytes) -> tuple[int, int, int, bytes]:
    """수신된 UDP 패킷을 (type, size, seq, body)로 분해한다."""
    msg_type, size, seq = struct.unpack_from(HEADER_FORMAT, data, 0)
    body = data[HEADER_SIZE: HEADER_SIZE + size]
    return msg_type, size, seq, body


# 파일 전송 / 수신

def send_file(sock: socket.socket, filename: str, dest: tuple):
    """파일을 UDP 패킷으로 분할 전송한다."""
    with open(filename, "rb") as f:
        seq = 0

        # 파일 시작 알림
        sock.sendto(make_packet(MSG_FILE_START, seq=seq), dest)
        seq += 1

        # 파일 데이터
        while True:
            chunk = f.read(CHUNK_SIZE)
            if not chunk:
                break
            sock.sendto(make_packet(MSG_FILE_DATA, chunk, seq), dest)
            seq += 1

        # 파일 종료 알림
        sock.sendto(make_packet(MSG_FILE_END, seq=seq), dest)

    print(f"File '{filename}' sent successfully.")


def receive_file(sock: socket.socket, output_filename: str = "received_file.dat"):
    """
    FILE_DATA / FILE_END 패킷을 받아 파일로 저장한다.
    FILE_START는 호출 전에 이미 소비되었다고 가정한다.
    """
    with open(output_filename, "wb") as f:
        while True:
            data, _ = sock.recvfrom(MAX_BUF)
            msg_type, size, seq, body = parse_packet(data)

            if msg_type == MSG_FILE_DATA:
                f.write(body)
            elif msg_type == MSG_FILE_END:
                print(f"File '{output_filename}' received successfully.")
                break