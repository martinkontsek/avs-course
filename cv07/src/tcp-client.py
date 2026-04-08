#!/usr/bin/env python3

import socket as s

IP = "::1"
PORT = 9999

if __name__ == "__main__":
    sock = s.socket(s.AF_INET6, s.SOCK_STREAM)
    sock.connect((IP,PORT))

    msg = input("Enter msg: ")
    sock.send(msg.encode())
    sock.close()
        
