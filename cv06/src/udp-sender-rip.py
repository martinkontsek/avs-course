#!/usr/bin/env python3

import socket as s
import struct

class RIP_hdr():
    def __init__(self):
        self._cmd = 2
        self._version = 2
        self._unused = 0
        self._entries = list()

    def add_entry(self, entry):
        self._entries.append(entry)
    
    def to_bytes(self):
        out = struct.pack("!BBH", self._cmd, self._version, self._unused)
        for entry in self._entries:
            out += entry.to_bytes()
        return out

class RIP_entry():
    def __init__(self, prefix, mask, next_hop, metric):
        self._afi = s.AF_INET
        self._route_tag = 0
        self._prefix = prefix
        self._mask = mask
        self._next_hop = next_hop
        self._metric = metric

    def to_bytes(self):
        out = struct.pack("!HH", self._afi, self._route_tag)
        out += s.inet_aton(self._prefix)
        out += s.inet_aton(self._mask)
        out += s.inet_aton(self._next_hop)
        out += struct.pack("!L", self._metric)
        return out
    

def sender(ip, port):
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM, 0)
    sock.bind(("192.168.1.1", 520))

    rip = RIP_hdr()
    rip.add_entry(RIP_entry("2.2.2.2", "255.255.255.255", "192.168.1.1", 1))
    rip.add_entry(RIP_entry("3.3.3.0", "255.255.255.255", "192.168.1.1", 1))
    
    sock.sendto(rip.to_bytes(), (ip, port))
    sock.close()

if __name__ == "__main__":
    sender("224.0.0.9", 520)