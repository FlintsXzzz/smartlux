#pragma once

// R6: MQTT + Home Assistant discovery — board-only glue.
//
// Semua keputusan murni (ID perangkat, topik, tabel entitas, payload discovery,
// backoff) ada di logic.h dan diuji di host. File ini hanya menjembatani WiFi,
// PubSubClient dan Serial. Ia sengaja TIDAK membaca g_net / g_lamp / variabel
// global firmware lain: include terjadi sebelum variabel itu dideklarasi.
// Panggilan masuk selalu lewat mqttWantsState() / mqttPoll() dari SMARTLUX.ino.
//
// Seluruh isi dikompilasi HANYA bila SMARTLUX_ENABLE_MQTT didefinisikan di
// secrets.h (atau -D pada build CI). Tanpa itu, build dasar tetap identik.
// Broker wajib lokal/VLAN tepercaya; data okupansi tidak boleh ke cloud.

#include "logic.h"

#if !defined(SMARTLUX_ENABLE_MQTT)

// No-op: memungkinkan .ino memanggil tanpa #if bersarang.
static inline void mqttBegin() {}
static inline bool mqttWantsState(bool) { return false; }
static inline void mqttPoll(bool, const char *, size_t) {}

#else  // SMARTLUX_ENABLE_MQTT

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#ifndef MQTT_BROKER
  #warning "SMARTLUX_ENABLE_MQTT aktif tetapi MQTT_BROKER belum diset; MQTT dinonaktifkan."
  #define SLX_MQTT_ACTIVE 0
#else
  #define SLX_MQTT_ACTIVE 1
#endif

#if defined(MQTT_USER) && !defined(MQTT_PASS)
  #error "MQTT_USER tanpa MQTT_PASS tidak didukung; set keduanya di secrets.h"
#endif

#ifndef MQTT_PORT
  #define MQTT_PORT 1883
#endif
#ifndef MQTT_BASE_TOPIC
  #define MQTT_BASE_TOPIC "smartlux"
#endif
#ifndef MQTT_DISCOVERY_PREFIX
  #define MQTT_DISCOVERY_PREFIX "homeassistant"
#endif

// Telemetri 5 detik, LWT retained QoS-1, backoff 5/5/10/20/30 detik.
static const uint32_t SLX_MQTT_TELEMETRY_MS = 5000UL;

static WiFiClient g_mqttWifi;
static PubSubClient g_mqtt(g_mqttWifi);

static char g_mqttId[7] = "";         // 6 hex + NUL
static char g_mqttBase[64] = "";
static char g_mqttState[80] = "";
static char g_mqttAvail[80] = "";
static char g_mqttClientId[40] = "";
static char g_mqttTopicBuf[128] = "";
static char g_mqttPayloadBuf[1024] = "";

static uint32_t g_mqttNextTryMs = 0;
static uint32_t g_mqttPublishAtMs = 0;
static uint32_t g_mqttBackoffMs = 0;
static unsigned int g_mqttAttempts = 0;
static bool g_mqttActive = false;
static bool g_mqttConnected = false;

static void mqttPublishState(const char *json, size_t len) {
  if (!json || len == 0) return;
  if (!g_mqtt.publish(g_mqttState, (const uint8_t *)json, (unsigned int)len, false)) {
    Serial.println(F("# MQTT publish state gagal"));
  }
}

// Config discovery di-publish retained SETIAP reconnect (HA bisa restart dan
// membaca ulang). Online retained ditulis setelah config agar status akhir
// selalu "online"; offline ditulis broker dari LWT bila perangkat mati.
static void mqttPublishDiscovery() {
  size_t count = 0;
  const MqttEntity *entities = mqttEntities(&count);
  for (size_t i = 0; i < count; ++i) {
    const MqttEntity *e = &entities[i];
    if (!mqttTopic(SLX_TOPIC_DISCOVERY, g_mqttId, e->component, e->object,
                   g_mqttTopicBuf, sizeof(g_mqttTopicBuf),
                   MQTT_BASE_TOPIC, MQTT_DISCOVERY_PREFIX)) {
      Serial.println(F("# MQTT discovery topic terpotong"));
      continue;
    }
    const int n = buildMqttDiscoveryConfig(e, g_mqttId, g_mqttPayloadBuf,
                                           sizeof(g_mqttPayloadBuf),
                                           MQTT_BASE_TOPIC);
    if (n < 0) {
      Serial.println(F("# MQTT discovery payload terpotong"));
      continue;
    }
    if (!g_mqtt.publish(g_mqttTopicBuf, (const uint8_t *)g_mqttPayloadBuf,
                        (unsigned int)n, true)) {
      Serial.printf("# MQTT discovery gagal: %s\n", g_mqttTopicBuf);
    }
  }
  g_mqtt.publish(g_mqttAvail, (const uint8_t *)"online", 6, true);
}

static void mqttBegin() {
#if SLX_MQTT_ACTIVE
  // WiFiClient TIDAK menyelesaikan ".local"; localhost mDNS bukan alamat broker.
  if (!mqttConfigured(MQTT_BROKER)) {
    Serial.println(F("# MQTT DINONAKTIFKAN: MQTT_BROKER belum diisi di secrets.h."));
    return;
  }

  const uint64_t mac = ESP.getEfuseMac();
  if (!mqttDeviceId(mac, g_mqttId, sizeof(g_mqttId))) return;
  if (!mqttTopic(SLX_TOPIC_BASE, g_mqttId, nullptr, nullptr, g_mqttBase,
                 sizeof(g_mqttBase), MQTT_BASE_TOPIC, MQTT_DISCOVERY_PREFIX) ||
      !mqttTopic(SLX_TOPIC_STATE, g_mqttId, nullptr, nullptr, g_mqttState,
                 sizeof(g_mqttState), MQTT_BASE_TOPIC, MQTT_DISCOVERY_PREFIX) ||
      !mqttTopic(SLX_TOPIC_AVAIL, g_mqttId, nullptr, nullptr, g_mqttAvail,
                 sizeof(g_mqttAvail), MQTT_BASE_TOPIC, MQTT_DISCOVERY_PREFIX)) {
    Serial.println(F("# MQTT DINONAKTIFKAN: topik terlalu panjang."));
    return;
  }
  snprintf(g_mqttClientId, sizeof(g_mqttClientId), "slx-%s-%04x", g_mqttId,
           (unsigned int)((mac >> 24) & 0xFFFFu));

  g_mqtt.setClient(g_mqttWifi);
  g_mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  // Payload discovery + atribut bisa ~850 B; 1024 default PubSubClient kurang.
  g_mqtt.setBufferSize(1536);
  // ESP32 core 3.x: WiFiClient = NetworkClient. Stream::setTimeout() hanya
  // mengatur baca/tulis, bukan TCP connect; ini yang membatasi connect 2 detik.
  g_mqttWifi.setConnectionTimeout(2000);

  g_mqttActive = true;
  g_mqttBackoffMs = 0;
  g_mqttNextTryMs = 0;
  Serial.printf("# MQTT aktif: %s:%u -> %s (id %s)\n",
                MQTT_BROKER, (unsigned int)MQTT_PORT, g_mqttBase, g_mqttId);
#else
  Serial.println(F("# MQTT DINONAKTIFKAN: MQTT_BROKER belum didefinisikan."));
#endif
}

// Apakah .ino perlu menyiapkan JSON status untuk loop ini? JSON dibangun hanya
// saat ada peluang publish/koneksi, supaya loop() tetap murah.
static inline bool mqttWantsState(bool ready) {
  if (!g_mqttActive || !ready) return false;
  const uint32_t now = millis();
  if (!g_mqttConnected)
    return mqttShouldAttempt(now, g_mqttNextTryMs, g_mqttBackoffMs);
  return mqttShouldAttempt(now, g_mqttPublishAtMs, SLX_MQTT_TELEMETRY_MS);
}

static void mqttPoll(bool ready, const char *stateJson, size_t len) {
  if (!g_mqttActive || !ready) return;
  const uint32_t now = millis();

  if (!g_mqttConnected) {
    if (!mqttShouldAttempt(now, g_mqttNextTryMs, g_mqttBackoffMs)) return;
    g_mqttNextTryMs = now;

    // Overload connect ini MENETAPKAN LWT (will) lebih dulu, lalu mengirim
    // CONNECT: retained "offline" QoS-1 bila perangkat putus mendadak.
    bool ok;
#if defined(MQTT_USER) && defined(MQTT_PASS)
    ok = g_mqtt.connect(g_mqttClientId, MQTT_USER, MQTT_PASS,
                        g_mqttAvail, 1, true, "offline");
#else
    ok = g_mqtt.connect(g_mqttClientId, nullptr, nullptr,
                        g_mqttAvail, 1, true, "offline");
#endif
    if (!ok) {
      ++g_mqttAttempts;
      g_mqttBackoffMs = mqttNextBackoffMs(g_mqttAttempts);
      Serial.println(F("# MQTT reconnect gagal"));
      return;
    }
    g_mqttAttempts = 0;
    g_mqttBackoffMs = 0;
    g_mqttConnected = true;
    mqttPublishDiscovery();
    mqttPublishState(stateJson, len);   // state segera setelah connect
    g_mqttPublishAtMs = now;
    Serial.println(F("# MQTT terhubung"));
    return;
  }

  g_mqtt.loop();   // keepalive + baca pesan (non-blocking)
  if (!g_mqtt.connected()) {
    g_mqttConnected = false;
    g_mqttNextTryMs = now;
    g_mqttBackoffMs = 0;
    g_mqttPublishAtMs = 0;   // publish state segera setelah reconnect
    Serial.println(F("# MQTT terputus"));
    return;
  }
  if (stateJson && len &&
      mqttShouldAttempt(now, g_mqttPublishAtMs, SLX_MQTT_TELEMETRY_MS)) {
    mqttPublishState(stateJson, len);
    g_mqttPublishAtMs = now;
  }
}

#endif  // SMARTLUX_ENABLE_MQTT
