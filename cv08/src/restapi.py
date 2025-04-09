#!/usr/bin/env python3

import socket as s
import struct
from fastapi import FastAPI, Response
import uvicorn
import threading
import time
from pydantic import BaseModel, Field
from pydantic.networks import IPvAnyAddress

app = FastAPI()

TIMER = 30
ROUTES = list()
NEXT_ID = 1

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
    
class RIP_entry_model(BaseModel):
    id: int | None = None
    prefix : IPvAnyAddress = "192.168.100.0"
    mask : IPvAnyAddress = "255.255.255.0"
    next_hop : IPvAnyAddress = "1.1.1.1"
    metric : int = Field(default= 1, ge=1, le=15)


def sender(ip, port):
    sock = s.socket(s.AF_INET, s.SOCK_DGRAM)
    # sock.setsockopt(s.IPPROTO_IP, s.IP_ADD_MEMBERSHIP, )
    sock.bind(("192.168.1.1", 520))

    while True:
        rip_hdr = struct.pack("!2BH", 2, 2, 0)
        
        rip_entry = bytes()
        for r in ROUTES:
            rip_entry += RIP_entry(r.prefix, r.mask, r.next_hop, r.metric).bytes()

        rip = rip_hdr + rip_entry
        print("RIP sent.")
        sock.sendto(rip, (ip, port))

        time.sleep(TIMER)

def init_routes():
    global NEXT_ID
    ROUTES.append(RIP_entry_model(id=NEXT_ID, prefix="3.3.3.0", mask="255.255.255.0", next_hop="192.168.1.1",metric=1))
    NEXT_ID += 1

@app.get("/")
def index():
    return "Hello."

@app.get("/api/routes")
def get_routes() -> list[RIP_entry_model]:
    return ROUTES

@app.post("/api/routes")
def add_route(route : RIP_entry_model) -> RIP_entry_model:
    global NEXT_ID
    route.id = NEXT_ID
    NEXT_ID += 1
    ROUTES.append(route)
    return route

@app.delete("/api/routes/{id}")
def delete_route(id: int, response : Response):
    for route in ROUTES:
        if route.id == id:
            ROUTES.remove(route)
            return route
    response.status_code = 404
    return {"error": "Item not found."}

@app.get("/api/timer")
def get_timer() -> int:
    return TIMER

@app.post("/api/timer")
def set_timer(timer : int) -> int:
    global TIMER
    TIMER = timer
    return TIMER

if __name__ == "__main__":
    init_routes()
    t = threading.Thread(target=sender, args=("224.0.0.9", 520))
    t.start()
    uvicorn.run(app, host="0.0.0.0", port=9000)
