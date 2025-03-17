#!/usr/bin/env python3

import socket as s

def sender(ip, port):
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
    # sock.setsockopt(s.IPPROTO_IP, s.IP_ADD_MEMBERSHIP, )

    msg = input("Enter message to send: ")
    sock.sendto(msg.encode(), (ip, port))

if __name__ == "__main__":
    sender("224.0.0.50", 1234)