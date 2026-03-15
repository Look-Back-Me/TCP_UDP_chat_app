import socket
import threading
import sys
from file_transfer import *


class UDPClient:
    def __init__(self, ip: str, port: int, name: str):
        self.sock   = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server = (ip, port)
        self.name   = name
        print(f"Connected to {ip}:{port} as {name}")

    def receive_messages(self):
        while True:
            data, _ = self.sock.recvfrom(MAX_BUF)
            if len(data) < HEADER_SIZE:
                continue

            msg_type, size, seq, body = parse_packet(data)

            if msg_type == MSG_CHAT:
                print(body.decode())

            elif msg_type == MSG_FILE_START:
                receive_file(self.sock, "downloaded_file.dat")

    def send_messages(self):
        while True:
            msg = input()
            if msg.lower() == "q":
                self.sock.close()
                sys.exit(0)

            elif msg.startswith("/sendfile "):
                fname = msg.split(" ", 1)[1]
                send_file(self.sock, fname, self.server)

            else:
                body = f"[{self.name}]: {msg}".encode()
                self.sock.sendto(make_packet(MSG_CHAT, body), self.server)

    def start(self) -> None:
        threading.Thread(target=self.receive_messages, daemon=True).start()
        self.send_messages()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: python3 {sys.argv[0]} <server_ip> <port> <name>")
        sys.exit(1)

    UDPClient(sys.argv[1], int(sys.argv[2]), sys.argv[3]).start()