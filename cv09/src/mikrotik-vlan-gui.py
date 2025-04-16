#!/usr/bin/env python3

import requests as r
from flask import Flask, render_template, request

HOST = "http://192.168.1.3"
USER = "admin"
PASS = ""
BRIDGE = "switch"

web_gui = Flask(__name__)

def get_switch_ifaces():
    resp = r.get(HOST+"/rest/int/br/port", auth=(USER,PASS))

    if resp.status_code == 200:
        out = list()
        for iface in resp.json():
            out.append({".id": iface[".id"], "interface": iface["interface"], "pvid": iface["pvid"]})
        return out
    
    return None

def set_pvid(iface, pvid):
    ifaces = get_switch_ifaces()
    for i in ifaces:
        if i["interface"] == iface:
            body = {"pvid": pvid}
            print(body)
            resp = r.patch(HOST+"/rest/int/br/port/"+i[".id"], auth=(USER,PASS), json=body)
            
            if resp.status_code == 200:
                return True
    return False

def add_vlan(vlan_id):
    body = {"bridge": BRIDGE, "vlan-ids": vlan_id}
    resp = r.put(HOST+"/rest/int/br/vlan", auth=(USER,PASS), json=body)

    if resp.status_code == 201:
        return True
    return False

def get_vlan():
    resp = r.get(HOST+"/rest/int/br/vlan", auth=(USER,PASS))

    if resp.status_code == 200:
        out = list()
        for iface in resp.json():
            out.append({".id": iface[".id"], "vlan-ids": iface["vlan-ids"], "tagged": iface["tagged"]})
        return out
    return None

@web_gui.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":
        print(request.form.keys())
        for key in request.form.keys():
            port = key.split("-")[0]
            type_pvid = key.split("-")[1]
            if type_pvid == "pvid":
                set_pvid(port, request.form[key])
    
    
    out = get_switch_ifaces()
    vids = get_vlan()
    for iface in out:
        iface["type"] = "access"

    return render_template("vlan.html", ifaces=out, vids=vids)


if __name__ == "__main__":
    # out = get_switch_ifaces() 
    # out = add_vlan(2)
    # out = get_vlan()
    # print(out)
    web_gui.run(host="0.0.0.0", port="8080")
    # set_pvid("e1", 1)