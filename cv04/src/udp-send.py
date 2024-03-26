#!/usr/bin/env python3

import socket as s

DST_IP = "224.0.0.9"
DST_PORT = 9999

sock = s.socket(s.AF_INET, s.SOCK_DGRAM)

msg = input("Enter message to send: ")

sock.sendto(msg.encode(), (DST_IP, DST_PORT))
sock.close()
