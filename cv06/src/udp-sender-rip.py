#!/usr/bin/env python3

import socket as s
import struct
import random

class RIP_entry():
    def __init__(self, prefix, mask, next_hop, metric):
        self._prefix = prefix
        self._mask = mask
        self._next_hop = next_hop
        self._metric = metric
    def bytes(self):
        out = struct.pack("!2H", 2, 0)
        out += s.inet_aton(self._prefix)
        out += s.inet_aton(self._mask)
        out += s.inet_aton(self._next_hop)
        out += struct.pack("!L", self._metric)
        return out

def sender(ip, port):
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
    # sock.setsockopt(s.IPPROTO_IP, s.IP_ADD_MEMBERSHIP, )
    sock.bind(("192.168.1.1", 520))

    rip_hdr = struct.pack("!2BH", 2, 2, 0)
    rip_entry = RIP_entry("3.3.3.0", "255.255.255.0", "192.168.1.1", 1).bytes()
    rip_entry += RIP_entry("4.4.4.4", "255.255.255.255", "192.168.1.1", 1).bytes()

    rip = rip_hdr + rip_entry
    sock.sendto(rip, (ip, port))

if __name__ == "__main__":
    sender("224.0.0.9", 520)