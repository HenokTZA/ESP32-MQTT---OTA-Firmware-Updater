/**
 * MQTT-driven OTA receiver (per-device topics)
 *  – requires ESP32 Arduino core ≥3.0
 *  – set MQTT_MAX_PACKET_SIZE big enough for 1 024-byte chunks
 */

#define MQTT_MAX_PACKET_SIZE 200       // <-- keep before PubSubClient
#include <WiFi.h>
#include <PubSubClient.h>
#include <Update.h>
#include "mbedtls/md5.h"

/* ---------- user settings ---------- */
#define WIFI_SSID   "Your_SSID"
#define WIFI_PASS   "Your_Password"
#define MQTT_HOST   "Broker_Port"     // PC / broker IP on hotspot
#define MQTT_PORT   1883  //Can use different broker port as needed
/* ----------------------------------- */

WiFiClient   net;
PubSubClient mqtt(net);

String   cid;                  // client-ID = chip-ID hex
String   topicOta;             // "ota/<cid>"
String   topicFb;              // "ota/feedback/<cid>"

size_t   expectedSize = 0;
bool     otaRunning   = false;

/* ---------- helpers ---------- */
void sendFB(const char *msg)
{
  /* retain = true → broker stores the last value until the PC client
     subscribes, eliminating the race without needing QoS 1 support */
  mqtt.publish(topicFb.c_str(), msg, /*retain=*/true);
}


bool md5Matches(const uint8_t *data, size_t len, const uint8_t *ref16) {
  uint8_t calc[16];
  mbedtls_md5(data, len, calc);
  return memcmp(calc, ref16, 16) == 0;
}

/* ---------- handle data packets ---------- */
void handleChunk(uint8_t *data, size_t len)
{
  if (!otaRunning) {                              // size message
    if (len != 4) return;
    expectedSize = (data[0]<<24)|(data[1]<<16)|(data[2]<<8)|data[3];
    Serial.printf("[OTA] Image size: %u bytes\n", (unsigned)expectedSize);

    if (!Update.begin(expectedSize)) { sendFB("begin-fail"); return; }
    otaRunning = true;
    sendFB("ok");                                 // ask for chunk 0
    return;
  }

  if (len < 20) { sendFB("len-fail"); return; }   // quick guard

  uint16_t payLen = (data[0]<<8)|data[1];
  if (len != 2 + payLen + 16 + 2) { sendFB("size-mismatch"); return; }

  const uint8_t *payload = data + 2;
  const uint8_t *md5     = data + 2 + payLen;
  uint16_t remaining     = (data[len-2]<<8)|data[len-1];

  if (!md5Matches(payload, payLen, md5)) { sendFB("md5-fail"); return; }

  if (Update.write((uint8_t*)payload, payLen) != payLen) {
    sendFB("write-fail"); return;
  }

  Serial.printf("[OTA] chunk OK – %u left\n", remaining);

  if (remaining == 0) {
    if (!Update.end(true)) { sendFB("end-fail"); return; }
    sendFB("success");
    Serial.println("[OTA] done → reboot");
    delay(500);
    ESP.restart();
  } else {
    sendFB("ok");
  }
}

/* ---------- MQTT callbacks / connect ---------- */
void cb(char *topic, uint8_t *payload, unsigned int len) {
  if (topicOta.equals(topic)) handleChunk(payload, len);
}

void ensureMqtt() {
  while (!mqtt.connected()) {
    Serial.printf("[MQTT] Connecting…\n");
    if (mqtt.connect(cid.c_str())) {
      Serial.println("[MQTT] Connected");
      mqtt.subscribe(topicOta.c_str(), 1);
      sendFB("ready");
    } else {
      Serial.printf("[MQTT] fail rc=%d\n", mqtt.state());
      delay(1000);
    }
  }
}

/* ---------- setup / loop ---------- */
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== OTA receiver ===");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) { Serial.print('.'); delay(500); }
  Serial.printf("\n[WiFi] IP: %s\n", WiFi.localIP().toString().c_str());

  cid      = "esp32-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);
  topicOta = "ota/" + cid;
  topicFb  = "ota/feedback/" + cid;

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(cb);
}

void loop() {
  ensureMqtt();
  mqtt.loop();
}
