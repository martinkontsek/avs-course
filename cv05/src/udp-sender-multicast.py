#!/usr/bin/env python3

import socket as s

def sender(ip, port):
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM, 0)

    buffer = input("Enter text to send: ")
    buffer += "\n"
    sock.sendto(buffer.encode(), (ip, port))
    sock.close()

if __name__ == "__main__":
    sender("224.0.0.2", 8080)