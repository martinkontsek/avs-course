#!/usr/bin/env python3

import socket
import threading

IP = "::"
PORT = 4321

def tcp_server(ip, port, backlog=2):
    try:
        sock = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
        sock.bind((ip, port))
        sock.listen(backlog)

        while True:
            (client_sock, addr) = sock.accept()
            print("Client [{}]:{} connected.".format(addr[0], addr[1]))

            threading.Thread(target=handle_client, args=(client_sock,)).start()
    except Exception as e:
        print("ERROR: "+str(type(e)) + str(e))
    
def handle_client(client_sock : socket.socket):
    buffer = client_sock.recv(100)
    print("MSG: " + buffer.decode())
    client_sock.close()


if __name__ == "__main__":
    tcp_server(IP, PORT)