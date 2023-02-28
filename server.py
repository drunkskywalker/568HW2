import socket 
import sys

import collections
import ipaddress
host = '127.0.0.1'
port = 80

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((host, port))
s.listen(1)

while True:
  client_socket, client_address = s.accept()
  data = client_socket.recv(1024)
  print('Received: ', data)
  if (data == b"GET / HTTP/1.1\r\nHost: 0.0.0.0\r\n\r\n"):
    client_socket.sendall(b"HTTP/1.1 200 OK\r\nETag: aaa\r\nCache-Control: max-age=1, no-cache\r\n\r\n")
  elif (data == b"GET /a HTTP/1.1\r\nHost: 0.0.0.0\r\n\r\n"):
    client_socket.sendall(b"HTTP/1.1 200 OK\r\nCache-Control: max-age=360000\r\n\r\n")
  else: 
    client_socket.send(b"HTTP/1.1 304 Not Modified\r\n\r\n")
  client_socket.close()
