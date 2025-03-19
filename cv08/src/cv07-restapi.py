#!/usr/bin/env python3
from fastapi import FastAPI, Response, status
import uvicorn
from pydantic import BaseModel, Field, PositiveInt
from pydantic.networks import IPvAnyAddress
import struct
import socket
from threading import Thread
import time
from fastapi.openapi.utils import get_openapi

RIP_IP = "224.0.0.9"
RIP_PORT = 520

class RIP():
    def __init__(self):
        self._id = 1
        self._routes = list()
        self._timer = 5

    def create_rip_hdr(self):
        return struct.pack("!BBH",2,2,0)
    
    def create_rip_routes(self):
        ret = bytes()
        for route in self._routes:
            ret += struct.pack("!HH", socket.AF_INET, 0) 
            ret += socket.inet_aton(str(route.prefix))
            ret += socket.inet_aton(str(route.mask))
            ret += socket.inet_aton(str(route.next_hop))
            ret += struct.pack("!I", route.metric)
        return ret

    def create_msg(self):
        return self.create_rip_hdr() + self.create_rip_routes()

class RIP_route(BaseModel):
    id: int | None = None
    prefix: IPvAnyAddress
    mask: IPvAnyAddress
    next_hop: IPvAnyAddress
    metric: int = Field(ge=0, le=16)

api = FastAPI(title="RIPv2 sender API")
rip = RIP()

@api.get("/")
def index():
    return "Hello"

@api.post("/v1/routes", status_code=status.HTTP_201_CREATED)
def create_route(route: RIP_route):
    route.id = rip._id
    rip._id += 1
    rip._routes.append(route)
    return route

@api.get("/v1/routes")
def get_routes() -> list[RIP_route]:
    return rip._routes

@api.get("/v1/routes/{id}")
def get_route(id: int, response: Response) -> RIP_route|None:
    for route in rip._routes:
        if route.id == id:
            return route
    
    response.status_code = status.HTTP_404_NOT_FOUND
    return None

@api.delete("/v1/routes/{id}")
def delete_route(id: int, response: Response) -> RIP_route|None:
    for route in rip._routes:
        if route.id == id:
            r = route
            rip._routes.remove(route)
            return r
        
    response.status_code = status.HTTP_404_NOT_FOUND
    return None

@api.patch("/v1/routes/{id}")
def edit_route(id: int, route: RIP_route, response: Response) -> RIP_route|None:
    for r in rip._routes:
        if r.id == id:
            r.prefix = route.prefix
            r.mask = route.mask
            r.next_hop = route.next_hop
            r.metric = route.metric
            return route
        
    response.status_code = status.HTTP_404_NOT_FOUND
    return None

@api.get("/v1/timer")
def get_timer() -> int:
    return rip._timer

@api.post("/v1/timer")
def set_timer(timer: PositiveInt) -> PositiveInt:
    rip._timer = int(timer)
    return rip._timer

def rip_sender_thread(sock: socket.socket):
    while True:
        rip_msg = rip.create_msg()
        sock.sendto(rip_msg, (RIP_IP, RIP_PORT))
        time.sleep(rip._timer)

def rip_sender():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    t = Thread(target=rip_sender_thread, args=(sock,))
    t.run()

def rip_main_thread():
    t = Thread(target=rip_sender)
    t.start()

def custom_openapi():
    if api.openapi_schema:
        return api.openapi_schema
    openapi_schema = get_openapi(
        title = "RIPv2 sender API",
        version="0.0.1",
        routes=api.routes,
    )
    prop = openapi_schema["components"]["schemas"]["RIP_route"]["properties"]
    prop["prefix"]["format"] = "ipv4"
    prop["mask"]["format"] = "ipv4"
    prop["next_hop"]["format"] = "ipv4"
    api.openapi_schema = openapi_schema
    return api.openapi_schema


api.openapi = custom_openapi

if __name__ == "__main__":  
    rip_main_thread() 
    uvicorn.run(app=api, host="0.0.0.0", port=9999)
