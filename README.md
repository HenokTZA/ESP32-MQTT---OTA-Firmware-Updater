# ESP32 MQTT → OTA Firmware Updater 🔄

**Zero-touch, broker-mediated, **_chunked_** firmware updates for fleets of ESP32 boards.**  
The project contains:

| Folder | Description |
| :----: | ----------- |
| `firmware/` | Arduino-style sketch that sits on the ESP32 and listens on its own MQTT topic (`ota/<chipId>`). |
| `host/`     | Python 3 script that breaks a `.bin` file into 200-byte chunks, publishes them, and listens for feedback on `ota/feedback/<chipId>`. |

<div align="center">
  <img src="docs/diagram.png" width="640" alt="System architecture">
</div>

---

## ✨ Features

* **Dynamic device discovery** – each board announces itself with a retained `ready` message.  
* **Integrity-checked chunks** – every 200-byte slice includes a 16-byte MD5 and “remaining chunks” counter.  
* **Any broker** – works with Mosquitto, EMQX, AWS IoT Core (just adjust TLS settings).  
* **One script → many boards** – update 1 or 100 devices in the same run.  
* **Minimal flash footprint** – uses the stock `Update` class; no external FS or HTTP server needed.  

---

## 🔧 Quick-start

### 1. Flash the receiver sketch

```cpp
// firmware/ota_receiver.ino – edit these first
#define WIFI_SSID "MyHotspot"
#define WIFI_PASS "SuperSecret"
#define MQTT_HOST "192.168.137.101"
#define MQTT_PORT 1883
Install ESP32 Arduino Core ≥ 3.0.

Set MQTT_MAX_PACKET_SIZE 200 in PubSubClient.h or a global -D build flag.

Compile & upload as usual.

2. Start a broker (local Mosquitto)
bash
Copy
Edit
docker run -it --rm -p 1883:1883 eclipse-mosquitto
Use a real CA / TLS listener in production.

3. Run the uploader
bash
Copy
Edit
cd host
python ota_uploader.py firmware/your_build.bin
When each board boots it will print:

csharp
Copy
Edit
=== OTA receiver ===
[WiFi] IP: 192.168.137.176
[MQTT] Connected
[OTA] Image size: ...
…and the uploader console shows progress per chip:

makefile
Copy
Edit
esp32-fc8ad4: ready
esp32-fc8ad4: ok 0/273
esp32-fc8ad4: ✓ finished
All devices done? The script exits automatically.

🖧 Topic map
Topic	Payload	Direction
ota/<chipId>	4-byte size or chunk	Python → ESP32
ota/feedback/<chipId>	ready, ok, success, or error string	ESP32 → Python

🚑 Troubleshooting
Symptom	Cause / Fix
len-fail right away	Chunk size too small → align CHUNK in Python with MQTT_MAX_PACKET_SIZE
size-mismatch	Broker truncates messages → confirm max-packet setting on all clients
Stalls at ok 0/…	Retain flag off? Ensure retain=True for feedback messages.
state() == -2 on connect	Wrong broker host / port / TLS mismatch.

🔐 Security considerations
TLS – replace WiFiClient with WiFiClientSecure, load CA root certificate.

Auth – user/password or X.509 client certs on the broker.

Signed images – extend the Python uploader to add an outer RSA or Ed25519 signature frame.

🗺️ Road-map / Future work
Idea	Status
Web drag-and-drop dashboard – Flask + Stomp.js front-end, drop a .bin, watch a live progress bar.	🟡 Planned
ESP32-S3 “dual-core” build flag detection.	⬜︎
Adaptive chunk size (512/1024) based on broker’s maximum-packet-size property.	⬜︎
GitHub Actions workflow: compile sketch & run pylint on uploader.	🟡
Test-bench Docker compose with Mosquitto + two ESP32 emulators (QEMU).	⬜︎

🤝 Contributing
Fork → feature branch → PR.

Follow the code style (clang-format for C++, ruff for Python).

Each PR must pass the CI build and include a short entry in CHANGELOG.md.

Bug reports → Issues (please attach serial log + uploader output).

📜 License
Released under the MIT License – see LICENSE for details.

yaml
Copy
Edit

---

### How to use the README

1. Save the block above as `README.md` at the repo root.  
2. Put a nice PNG/SVG of your architecture in `docs/diagram.png` (or remove that `<img>` tag).  
3. Push it – GitHub automatically renders the Markdown.

That’s it! You now have a clean, documented, contribution-ready project. As you add the drag-and-drop web
