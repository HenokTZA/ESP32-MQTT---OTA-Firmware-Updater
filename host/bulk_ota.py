#!/usr/bin/env python3
"""
Bulk OTA uploader (per-device topics)
  • publishes size + 1 024-byte chunks on ota/<chipId>
  • listens on ota/feedback/<chipId>
"""

import argparse, pathlib, hashlib, struct, time, sys, threading, re
import collections
import paho.mqtt.client as mqtt

# ── config ────────────────────────────────────────────────────────────────────
BROKER  = "192.168.137.101"       # same machine that runs Mosquitto
PORT    = 1883
CHUNK   = 200
QOS     = 1
# ──────────────────────────────────────────────────────────────────────────────

# ── CLI / firmware path ───────────────────────────────────────────────────────
p = argparse.ArgumentParser()
p.add_argument("fw", nargs="?", default="firmware.ino.bin",
               help="firmware image (default: firmware.bin)")
args = p.parse_args()
FW_PATH = pathlib.Path(args.fw)
fw      = FW_PATH.read_bytes()
print(f"Firmware: {len(fw)} bytes → ", end="")
# ──────────────────────────────────────────────────────────────────────────────

# ── build chunk list ──────────────────────────────────────────────────────────
num_chunks = (len(fw)+CHUNK-1)//CHUNK
print(f"{num_chunks} chunks")

def make_chunk(index: int) -> bytes:
    payload = fw[index*CHUNK:(index+1)*CHUNK]
    size    = len(payload).to_bytes(2,"big")
    md5     = hashlib.md5(payload).digest()
    remain  = (num_chunks-index-1).to_bytes(2,"big")
    return size + payload + md5 + remain

chunks = [make_chunk(i) for i in range(num_chunks)]
# ──────────────────────────────────────────────────────────────────────────────

# ── per-device state ──────────────────────────────────────────────────────────
Dev   = collections.namedtuple("Dev", "idx finished")
state = {}                       # chipId -> Dev
done  = set()

FB_RE = re.compile(r"ota/feedback/(.+)")


def on_msg(cli, _, msg):
    m = FB_RE.match(msg.topic)
    if not m:
        return
    chip = m.group(1)
    fb   = msg.payload.decode()

    if fb == "ready":
        print(f"{chip}: ready")
        cli.publish(f"ota/{chip}", len(fw).to_bytes(4,"big"), qos=QOS)
        state[chip] = Dev(0, False)           # ensure entry exists

    elif fb == "ok":
        dev = state[chip]                     # SAFE now – always exists
        if dev.idx < num_chunks:
            cli.publish(f"ota/{chip}", chunks[dev.idx], qos=QOS)
            state[chip] = Dev(dev.idx + 1, False)
        print(f"{chip}: ok {dev.idx}/{num_chunks}")

    elif fb == "success":
        print(f"{chip}: ✓ finished")
        state[chip] = Dev(state[chip].idx, True)
        if all(d.finished for d in state.values()):
            print("All devices finished – exiting.")
            sys.exit(0)
    else:
        print(f"{chip}: error → {fb}")



def on_connect(cli, _, __, rc, *a):
    if rc == 0:
        print("Connected to broker")
        cli.subscribe("ota/feedback/#", qos=QOS)
    else:
        print(f"Connect failed rc={rc}")

# ── MQTT client ───────────────────────────────────────────────────────────────
client = mqtt.Client("ota-uploader")
client.on_connect  = on_connect
client.on_message  = on_msg
client.enable_logger()

client.connect(BROKER, PORT, 60)

print("Uploader running – Ctrl-C to abort")
client.loop_forever()
