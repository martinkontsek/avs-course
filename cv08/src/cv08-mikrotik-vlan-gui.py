
import requests
from flask import Flask, render_template, Response, request

IP = "10.0.0.2"
USER = "admin"
PASS = "admin"
BRIDGE = "br1"

web_gui = Flask(__name__)

def get_interfaces():
    resp = requests.get("http://{}/rest{}".format(IP, "/int"), auth=(USER, PASS))
    if resp.status_code == 200:
        for i in resp.json():
            print(i["name"])

def get_vlans():
    resp = requests.get("http://{}/rest{}".format(IP, "/int/br/vlan"), auth=(USER, PASS))
    if resp.status_code == 200:
        return resp.json()
    else:
        return None
    
def create_vlan(name, vlan_id):
    body = {"bridge": BRIDGE, "comment": name, "vlan-ids": vlan_id}
    resp = requests.put("http://{}/rest{}".format(IP, "/int/br/vlan"),
                         auth=(USER, PASS),
                         json=body)
    if resp.status_code == 201:
        return resp.json()
    else:
        return None
    
def set_untagged(interface, vlan_id):
    ports = get_bridge_ports()
    if ports == None:
        return False
    
    port_id = None
    for port in ports:
        if port["interface"] == interface:
            port_id = port[".id"]
    if port_id == None:
        return False

    body = {"pvid": vlan_id}
    resp = requests.patch("http://{}/rest{}/{}".format(IP, "/int/br/port", port_id),
                         auth=(USER, PASS),
                         json=body)
    return resp.status_code == 200

def set_tagged(interface, vlan_num):
    vlans = get_vlans()
    if vlans == None:
        return False
    
    vlan_id = None
    tagged = ""
    for vlan in vlans:
        if vlan["vlan-ids"] == str(vlan_num):
            vlan_id = vlan[".id"]
            tagged = vlan["tagged"]
            if tagged != "":
                tagged += ","
    if vlan_id == None:
        return False

    body = {"tagged": tagged+interface}
    url = "http://{}/rest{}/{}".format(IP, "/int/br/vlan", vlan_id)
    print(url)
    resp = requests.patch(url,
                         auth=(USER, PASS),
                         json=body)
    print(resp.status_code)
    print(resp.json())
    return resp.status_code == 200

    
def get_bridge_ports():
    resp = requests.get("http://{}/rest{}".format(IP, "/int/br/port"), auth=(USER, PASS))
    if resp.status_code == 200:
        return resp.json()
    else:
        return None
    

@web_gui.get("/")
def index():
    return get_vlans()

@web_gui.route("/vlans", methods=["GET", "POST"])
def vlan_gui():
    if request.method == "GET":
        return render_template("vlan.html", 
                            ports=get_bridge_ports(), 
                            vlans=get_vlans())
    if request.method == "POST":
        request.form["ether2|2"]


if __name__ == "__main__":
    web_gui.run("0.0.0.0", port=9999)
    # set_tagged("ether3", 3)
    # create_vlan("test2", 3)
    # print(get_vlans())
    # set_untagged("ether2", 2)
    # print(get_bridge_ports())