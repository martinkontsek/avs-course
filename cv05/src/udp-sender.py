#!/usr/bin/env python3
import socket as s

IP = "224.0.0.10"
PORT = 9999

def send_udp(ip, port, msg):
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
    sock.sendto(msg.encode(), (ip, port))

if __name__ == "__main__":
    msg = input("Enter msg to send: ")
    send_udp(IP, PORT, msg)