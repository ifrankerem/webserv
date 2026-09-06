import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 8080))

s.sendall(b"GET / HTTP/1.1\r\nHost: local")

time.sleep(5)

s.sendall(b"host\r\n\r\n")

print(s.recv(4096).decode(errors="replace"))

s.close()