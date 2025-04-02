#!/usr/bin/env python3

import socket as s
import threading as t

IP = "::"
PORT = 4200

def handle_client(sock : s.socket):
    buffer_bytes = sock.recv(1024)
    print("MSG: "+buffer_bytes.decode())
    sock.close()

def tcp_server(ip, port, backlog=2):
    sock = s.socket(s.AF_INET6, s.SOCK_STREAM)
    sock.bind((ip, port))
    sock.listen(backlog)

    while True:
        (client_sock, addr) = sock.accept()
        print("Connected client [{}]:{}.".format(addr[0], addr[1]))

        t.Thread(target=handle_client, args=(client_sock,)).start()

    sock.close()

if __name__ == "__main__":
    tcp_server(IP, PORT)