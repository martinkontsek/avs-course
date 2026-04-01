#!/usr/bin/env python3

import socket as s
import struct

class RIP():
    def __init__(self):
        self._entries = list()
    
    def add_entry(self, entry):
        self._entries.append(entry)

    def to_bytes(self):
        out = struct.pack("!BBH", 2, 2, 0)
        for entry in self._entries:
            out += entry.to_bytes()
        return out
    
class RIP_entry():
    def __init__(self, ip, mask, nh, metric):
        self._afi = s.AF_INET
        self._rt = 0
        self._ip = ip
        self._mask = mask
        self._nh = nh
        self._metric = metric
    
    def to_bytes(self):
        out = struct.pack("!HH", self._afi, self._rt)
        out += s.inet_aton(self._ip)
        out += s.inet_aton(self._mask)
        out += s.inet_aton(self._nh)
        out += struct.pack("!I", self._metric)
        return out

if __name__ == "__main__":
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
    sock.bind(("192.168.1.1", 520))

    rip = RIP()
    rip.add_entry(RIP_entry("2.2.2.2", "255.255.255.255","3.3.3.3", 2))

    sock.sendto(rip.to_bytes(), ("224.0.0.9", 520))
    sock.close()