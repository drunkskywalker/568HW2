import socket 
import json
import sys
import ipaddress

s = socket.socket()
host = '127.0.0.1'
port = 12345
addr = ipaddress.ip_address(host)
family = socket.AF_INET if addr.version == 4 else socket.AF_INET6
s.connect((host, port))
message = b"GET / HTTP/1.1\r\nHost: 0.0.0.0\r\n\r\n"
s.send(message)
reply = s.recv(65536)
print(reply)
s.close()

s = socket.socket()
host = '127.0.0.1'
port = 12345
addr = ipaddress.ip_address(host)
family = socket.AF_INET if addr.version == 4 else socket.AF_INET6
s.connect((host, port))
message = b"GET / HTTP/1.1\r\nHost: 0.0.0.0\r\n\r\n"
s.send(message)
reply = s.recv(65536)
print(reply)
s.close()

s = socket.socket()
host = '127.0.0.1'
port = 12345
addr = ipaddress.ip_address(host)
family = socket.AF_INET if addr.version == 4 else socket.AF_INET6
s.connect((host, port))
message = b"GET /a HTTP/1.1\r\nHost: 0.0.0.0\r\n\r\n"
s.send(message)
reply = s.recv(65536)
print(reply)
s.close()

s = socket.socket()
host = '127.0.0.1'
port = 12345
addr = ipaddress.ip_address(host)
family = socket.AF_INET if addr.version == 4 else socket.AF_INET6
s.connect((host, port))
message = b"GET /a HTTP/1.1\r\nHost: 0.0.0.0\r\n\r\n"
s.send(message)
reply = s.recv(65536)
print(reply)
s.close()