#!/usr/bin/env python3

import requests as r
from flask import Flask, render_template, request, Response

HOST = "http://192.168.1.3/rest"
USER = "admin"
PASS = ""

def get_switch_ifaces():
    resp = r.get(HOST+"/int/bri/port", auth=(USER, PASS))

    if resp.status_code == 200:
        out = list()
        for iface in resp.json():
            out.append({"id": iface[".id"],
                        "name": iface["interface"],
                        "pvid": iface["pvid"]
                       })
        return out
    else:
        print(resp.status_code)
        return None
    
def get_iface_id(iface_name):
    for iface in get_switch_ifaces():
        if iface["name"] == iface_name:
            return iface["id"]
    return None


def set_iface_pvid(iface_name, pvid):
    id = get_iface_id(iface_name)
    if id == None:
        return False
    
    body = {"pvid": pvid}
    resp = r.patch(HOST+"/int/bri/port/"+id, auth=(USER, PASS), json=body)
    if resp.status_code == 200:
        return True
    else:
        print(resp.status_code)
        print(resp.json())
        return False


gui = Flask(__name__)

@gui.route("/", methods=["GET", "POST"])
def index():
    if request.method == "GET":
        ifaces = get_switch_ifaces()
        return render_template("vlan.html", iface_list=ifaces)
    else:
        print(request.form.keys())
        for name in request.form.keys():
            out = set_iface_pvid(name, request.form.get(name))
            if out == False:
                ifaces = get_switch_ifaces()
                return render_template("vlan.html", iface_list=ifaces, status=False)
        
        ifaces = get_switch_ifaces()
        return render_template("vlan.html", iface_list=ifaces, status=True)



if __name__ == "__main__":
    # ifaces = get_switch_ifaces()
    # print(ifaces)

    # set_iface_pvid("e2", 10)

    gui.run(host="0.0.0.0", port=9999)