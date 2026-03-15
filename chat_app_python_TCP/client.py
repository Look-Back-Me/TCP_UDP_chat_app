import socket
import threading
import sys
from file_transfer import FileTransfer, MSG_CHAT, MSG_FILE_START


class TCPClient:
    def __init__(self, ip, port, name):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((ip, port))
        self.name = name
        print(f"Connected to {ip}:{port} as {name}")

    def receive_messages(self):
        while True:
            header = FileTransfer.recv_header(self.sock)
            if not header:
                break
            msg_type, size, seq = header

            if msg_type == MSG_CHAT:
                msg = FileTransfer.recv_all(self.sock, size).decode()
                print(msg)
            elif msg_type == MSG_FILE_START:
                FileTransfer.receive_file(self.sock, "downloaded_file.dat")

    def send_messages(self):
        while True:
            msg = input()
            if msg.lower() == "q":
                self.sock.close()
                sys.exit(0)
            elif msg.startswith("/sendfile "):
                fname = msg.split(" ", 1)[1]
                FileTransfer.send_file(self.sock, fname)
            else:
                full_msg = f"[{self.name}]: {msg}"
                FileTransfer.send_header(self.sock, MSG_CHAT, len(full_msg), 0)
                self.sock.sendall(full_msg.encode())

    def start(self):
        threading.Thread(target=self.receive_messages, daemon=True).start()
        self.send_messages()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: python3 {sys.argv[0]} <server_ip> <port> <name>")
        sys.exit(1)

    TCPClient(sys.argv[1], int(sys.argv[2]), sys.argv[3]).start()