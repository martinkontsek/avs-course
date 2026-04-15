#!/usr/bin/env python3

import socket as s
import struct
import threading as t
import time
from fastapi import FastAPI, Response
from pydantic import BaseModel, Field
from pydantic.networks import IPvAnyAddress, IPvAnyNetwork
import uvicorn 

TIMER = 30
ROUTES = list()
API_HOST = "0.0.0.0"
API_PORT = 9999
RIP_IP = "192.168.1.1"
LAST_ID = 0

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
    
def RIP_sender():
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
    sock.bind((RIP_IP, 520))

    while True:
        rip = RIP()
        for r in ROUTES:
            rip.add_entry(RIP_entry(str(r.prefix.network_address), str(r.prefix.netmask), str(r.next_hop), r.metric))

        sock.sendto(rip.to_bytes(), ("224.0.0.9", 520))

        time.sleep(TIMER)


    sock.close()


################# API PART
class Timer(BaseModel):
    timer : int = Field(default=30, ge=1, le=60)

class RIP_route_model(BaseModel):
    id: int = 0
    prefix: IPvAnyNetwork
    next_hop: IPvAnyAddress
    metric: int = Field(ge=1, le=15)

api = FastAPI()

@api.get("/")
def index():
    return "API"

@api.get("/rip/timer")
def get_timer() -> Timer: 
    global TIMER
    return Timer(timer=TIMER)

@api.put("/rip/timer")
def set_timer(t: Timer) -> Timer:
    global TIMER
    TIMER = t.timer
    return Timer(timer=TIMER)

@api.put("/rip/routes")
def add_route(r: RIP_route_model) -> RIP_route_model:
    global LAST_ID
    r.id = LAST_ID
    ROUTES.append(r)
    LAST_ID += 1
    return r

@api.get("/rip/routes")
def get_routes() -> list[RIP_route_model]:
    return ROUTES

@api.delete("/rip/routes/{ID}")
def remove_route(ID : int, resp: Response) -> RIP_route_model | str:
    for i in range(0, len(ROUTES)):
        if ROUTES[i].id == ID:
            return ROUTES.pop(i)
    resp.status_code = 404
    return "ID not found."
    
def seed_routes():
    global LAST_ID
    ROUTES.append(RIP_route_model(id=LAST_ID, prefix="192.168.1.0/24", next_hop="0.0.0.0", metric=1))
    LAST_ID += 1
    ROUTES.append(RIP_route_model(id=LAST_ID, prefix="192.168.2.0/24", next_hop="0.0.0.0", metric=1))
    LAST_ID += 1

if __name__ == "__main__":
    seed_routes()

    thread = t.Thread(target=RIP_sender, args=())
    thread.start()

    uvicorn.run(app=api, host=API_HOST, port=API_PORT)