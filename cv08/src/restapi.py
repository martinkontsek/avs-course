#!/usr/bin/env python3

import socket as s
import struct
import time
import threading
from fastapi import FastAPI, Response
import uvicorn
from pydantic import BaseModel, Field, IPvAnyAddress

TIMER = 30
API = FastAPI()
ROUTES = list()
NEXT_ID = 1

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

    while True:
        rip = RIP_hdr()

        for r in ROUTES:
            rip.add_entry(RIP_entry(str(r.prefix), str(r.mask), str(r.next_hop), r.metric))                        
        
        sock.sendto(rip.to_bytes(), (ip, port))
        time.sleep(TIMER)

    sock.close()

class Timer(BaseModel):
    timer : int = Field(default=30, ge=1, le=60)

class RIP_route_model(BaseModel):
    id: int | None = None
    prefix : IPvAnyAddress
    mask : IPvAnyAddress
    next_hop : IPvAnyAddress
    metric : int = Field(default=1, ge=1, lt=16)

class API_error(BaseModel):
    error : str

@API.get("/api/timer")
def get_timer() -> Timer:
    return Timer(timer=TIMER)

@API.put("/api/timer")
def set_timer(t : Timer) -> Timer:
    global TIMER
    TIMER = t.timer
    return Timer(timer=TIMER)

@API.put("/api/timer1")
def set_timer1(t : int) -> Timer:
    global TIMER
    TIMER = t
    return Timer(timer=TIMER)

@API.get("/api/routes")
def get_routes() -> list[RIP_route_model]:
    return ROUTES

@API.post("/api/routes")
def add_route(r : RIP_route_model) -> RIP_route_model:
    global NEXT_ID
    r.id = NEXT_ID
    NEXT_ID += 1
    ROUTES.append(r)
    return r

@API.delete("/api/routes")
def delete_route(id : int, resp : Response) -> RIP_route_model | API_error:
    for route in ROUTES:
        if route.id == id:
            ROUTES.remove(route)
            return route
    resp.status_code = 404
    # return {"error": "Route with ID {} not found.".format(id)}
    return API_error(error="Route with ID {} not found.".format(id))

def init_routes():
    global NEXT_ID
    ROUTES.append(RIP_route_model(id=NEXT_ID, prefix="2.2.2.2", mask="255.255.255.255", next_hop="192.168.1.1", metric=1))
    NEXT_ID += 1
    ROUTES.append(RIP_route_model(id=NEXT_ID, prefix="3.3.3.0", mask="255.255.255.255", next_hop="192.168.1.1", metric=1))
    NEXT_ID += 1

if __name__ == "__main__":
    init_routes()
    t = threading.Thread(target=sender, args=("224.0.0.9", 520))
    t.start()
    uvicorn.run(app=API, host="0.0.0.0", port=8000)

