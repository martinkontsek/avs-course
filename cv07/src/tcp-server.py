#!/usr/bin/env python3

import socket as s
import threading as t

IP = "::"
PORT = 9999
BACKLOG = 1

def handle_client(client_sock):
    while True:
        recv_text = client_sock.recv(100)
        if len(recv_text) == 0:
            break
        print("MSG: "+recv_text.decode())

if __name__ == "__main__":
    sock = s.socket(s.AF_INET6, s.SOCK_STREAM)
    sock.bind((IP,PORT))
    sock.listen(BACKLOG)

    while True:
        (client_sock, client_addr) = sock.accept()

        print("Connected client [{}:{}]."
              .format(client_addr[0], client_addr[1]))
        
        thr = t.Thread(target=handle_client, args=(client_sock,))
        thr.start()
        
    sock.close()