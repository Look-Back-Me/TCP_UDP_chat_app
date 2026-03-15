import struct

CHUNK_SIZE = 65536  # 64KB

HEADER_FORMAT = "iii"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)

MSG_CHAT = 1
MSG_FILE_START = 2
MSG_FILE_DATA = 3
MSG_FILE_END = 4


class FileTransfer:
    @staticmethod
    def recv_all(sock, length):
        data = b""
        while len(data) < length:
            packet = sock.recv(length - len(data))
            if not packet:
                return None
            data += packet
        return data

    @staticmethod
    def send_header(sock, msg_type, size, seq):
        header = struct.pack(HEADER_FORMAT, msg_type, size, seq)
        sock.sendall(header)

    @staticmethod
    def recv_header(sock):
        data = FileTransfer.recv_all(sock, HEADER_SIZE)
        if not data:
            return None
        return struct.unpack(HEADER_FORMAT, data)

    @staticmethod
    def send_file(sock, filename):
        with open(filename, "rb") as f:
            seq = 0
            FileTransfer.send_header(sock, MSG_FILE_START, 0, seq)
            seq += 1

            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                FileTransfer.send_header(sock, MSG_FILE_DATA, len(chunk), seq)
                sock.sendall(chunk)
                seq += 1

            FileTransfer.send_header(sock, MSG_FILE_END, 0, seq)

        print(f"File '{filename}' sent successfully.")

    @staticmethod
    def receive_file(sock, output_filename="received_file.dat"):
        with open(output_filename, "wb") as f:
            while True:
                header = FileTransfer.recv_header(sock)
                if not header:
                    break
                msg_type, size, seq = header

                if msg_type == MSG_FILE_DATA:
                    chunk = FileTransfer.recv_all(sock, size)
                    if not chunk:
                        break
                    f.write(chunk)
                elif msg_type == MSG_FILE_END:
                    print(f"File '{output_filename}' received successfully.")
                    break