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

### 2. The .bin file should be kept in she same folder as the python script
 ```cpp
p.add_argument("fw", nargs="?", default="firmware.ino.bin",
               help="firmware image (default: firmware.bin)")
Change the default firmware.ino.bin with actual name of your .bin file
