#!/usr/bin/env python3

import socket as s
import threading as t

BIND_IP = "0.0.0.0"
BIND_PORT = 9999

def handle_client(client_sock : s.socket):
    while True:
        buf = client_sock.recv(1500)

        #client ended connection
        if len(buf) == 0:
            break

        print("Msg: "+buf.decode())

if __name__ == "__main__":

    sock = s.socket(s.AF_INET, s.SOCK_STREAM)
    sock.bind((BIND_IP, BIND_PORT))
    sock.listen(10)
    
    while True:
        (client_sock, (ip, port)) = sock.accept()
        print("Connected client {}:{}".format(ip, port))

        thread = t.Thread(target=handle_client, args=(client_sock,))
        thread.start()


