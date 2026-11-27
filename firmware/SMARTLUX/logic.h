// logic.h - pure, host-testable logic for SMARTLUX.
//
// No Arduino dependencies: this header is included by SMARTLUX.ino (so the
// board and the host run the same code) and by tests/native (so the rules are
// covered by unit tests without hardware).
#pragma once

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Dashboard auth token: SMARTLUX_TOKEN from secrets.h, or a 12-hex MAC fallback.
#define SLX_TOKEN_MAX_LEN 64u

// Shared mode codes so pure helpers never hard-code a literal. SMARTLUX.ino
// static_asserts that its enum matches these values; this header asserts the
// same contract so host tests catch a drift too.
#define SLX_MODE_AUTO 0
#define SLX_MODE_MANUAL_ON 1
#define SLX_MODE_MANUAL_OFF 2
#define SLX_MODE_COUNT 3
static_assert(SLX_MODE_AUTO == 0 && SLX_MODE_MANUAL_ON == 1 &&
                  SLX_MODE_MANUAL_OFF == 2 &&
                  SLX_MODE_COUNT == SLX_MODE_MANUAL_OFF + 1,
              "logic.h mode codes must stay AUTO=0, ON=1, OFF=2");

// Mode encoding matches `enum Mode` in SMARTLUX.ino:
//   AUTO = 0, MANUAL_ON = 1, MANUAL_OFF = 2
// Returns whether the lamp should be ON for the given sensors and mode.
static inline bool lampShouldBeOn(int mode, bool pirReady, bool dark, bool motion) {
  if (!pirReady) return false;                   // PIR warm-up: force a safe OFF
  if (mode == SLX_MODE_MANUAL_ON) return true;   // MANUAL_ON ignores the sensors
  if (mode == SLX_MODE_MANUAL_OFF) return false; // MANUAL_OFF ignores the sensors
  return dark && motion;                         // AUTO: dark AND motion (delay-off by caller)
}

// Next mode when the override button cycles: AUTO -> ON -> OFF -> AUTO.
static inline int cycleModeValue(int mode) { return (mode + 1) % SLX_MODE_COUNT; }

// Parse a /api/override `mode` argument. Returns AUTO/ON/OFF, or -1 when invalid.
static inline int parseMode(const char *m) {
  if (!m) return -1;
  if (!strcmp(m, "auto")) return SLX_MODE_AUTO;
  if (!strcmp(m, "on")) return SLX_MODE_MANUAL_ON;
  if (!strcmp(m, "off")) return SLX_MODE_MANUAL_OFF;
  return -1;
}

// JSON/log key for a mode: "auto" | "on" | "off".
static inline const char *modeKeyFor(int mode) {
  if (mode == SLX_MODE_MANUAL_ON) return "on";
  if (mode == SLX_MODE_MANUAL_OFF) return "off";
  return "auto";
}

// LDR hysteresis: -1 = keep previous state, 0 = dark, 1 = bright.
static inline int lightDecision(int adc, int darkOn, int brightOff) {
  if (adc < darkOn) return 0;
  if (adc > brightOff) return 1;
  return -1;
}

// Clamp a 12-bit ADC reading into 0..4095 (shared LDR input range). Keeping
// this in one place means powf() can never see a negative base.
static inline int clampAdc(int adc) {
  if (adc < 0) return 0;
  if (adc > 4095) return 4095;
  return adc;
}

// Map a 12-bit ADC reading to 0..100 %, clamped.
static inline int lightPercent(int adc) {
  return (int)((long)clampAdc(adc) * 100L / 4095L);
}

// Rough lux estimate for the non-linear LDR; full scale at ADC 4095.
static inline float luxEstimate(int adc, float fullScale) {
  const float ratio = (float)clampAdc(adc) / 4095.0f;
  return fullScale * powf(ratio, 2.2f);
}

// True while the delay-off window since the last motion is still open.
static inline bool motionActive(unsigned long now, unsigned long lastMotionMs,
                                unsigned long delayMs) {
  return (now - lastMotionMs) < delayMs;
}

// Debounced level for the override button (true = HIGH = released). `previous`
// is the last stable level; `raw` is accepted only after staying unchanged for
// debounceMs. Unsigned subtraction keeps millis() rollover harmless.
static inline bool stableButtonState(bool raw, bool previous,
                                     unsigned long nowMs,
                                     unsigned long lastChangeMs,
                                     unsigned long debounceMs) {
  if ((nowMs - lastChangeMs) < debounceMs) return previous;
  return raw;
}

// Energy for one sample: V * I * dt, with dt in milliseconds and I in mA.
static inline double energyIncrementWh(float volts, float milliamps,
                                       unsigned long dtMs) {
  return (double)volts * ((double)milliamps / 1000.0) *
         ((double)dtMs / 3600000.0);
}

// Savings versus an always-on lamp, clamped to 0..100 %. Elapsed in ms.
static inline double computeSavingsPct(double lampPowerW, double lampEnergyWh,
                                       unsigned long elapsedMs) {
  const double hours = (double)elapsedMs / 3600000.0;
  const double baseline = lampPowerW * hours;
  if (baseline < 1e-6) return 0.0;
  double s = (baseline - lampEnergyWh) / baseline * 100.0;
  if (s < 0.0) s = 0.0;
  if (s > 100.0) s = 100.0;
  return s;
}

// --- ADC median filter (R1) --------------------------------------------------
// Median of up to the first 5 samples; rejects single-sample spikes.
// n <= 0 returns 0. Even n uses the upper middle element.
static inline int adcMedian5(const int *s, int n) {
  if (n <= 0 || s == nullptr) return 0;
  if (n > 5) n = 5;
  int c[5];
  for (int i = 0; i < n; i++) c[i] = s[i];
  for (int i = 1; i < n; i++) {
    int v = c[i], j = i - 1;
    while (j >= 0 && c[j] > v) { c[j + 1] = c[j]; j--; }
    c[j + 1] = v;
  }
  return c[n / 2];
}

// --- Two-level lamp brightness (R4) -------------------------------------------
// PWM duty 0..255. Off -> 0; MANUAL_ON -> 255 always. AUTO: full 255 until the
// last `dimMs` of the off-delay window, then 128; expired -> 0.
static inline int lampBrightness(bool lampOn, int mode, unsigned long nowMs,
                                 unsigned long lastMotionMs,
                                 unsigned long delayMs, unsigned long dimMs) {
  if (!lampOn) return 0;
  if (mode == SLX_MODE_MANUAL_ON) return 255;
  const unsigned long elapsed = nowMs - lastMotionMs;
  if (elapsed >= delayMs) return 0;
  return (delayMs - elapsed) <= dimMs ? 128 : 255;
}

// --- Tier 1: BH1750 lux, runtime config, savings uncertainty -----------------

// BH1750 H-resolution mode: lux = raw / 1.2 (datasheet).
static inline float bh1750Lux(uint16_t raw) {
  return (float)raw / 1.2f;
}

// Hysteresis on measured lux: -1 keep, 0 dark, 1 bright.
static inline int lightDecisionLux(float lux, float darkOn, float brightOff) {
  if (lux < darkOn) return 0;
  if (lux > brightOff) return 1;
  return -1;
}

static inline bool validDelaySeconds(int seconds) {
  return seconds >= 5 && seconds <= 200;
}

static inline bool pwmFrequencyAllowed(int hz) {
  return hz == 100 || hz == 500 || hz == 1000 || hz == 5000;
}

// Uncertainty of the savings percentage, in percentage points. Both the
// measured lamp energy and the calculated always-on baseline carry relative
// error; the ratio's error is their quadrature sum. Returns 0 when there is
// no elapsed time or baseline yet.
static inline double savingsMarginPct(double lampPowerW, double lampEnergyWh,
                                      unsigned long elapsedMs,
                                      double lampEnergyErrorPct,
                                      double baselineErrorPct) {
  const double baseline = lampPowerW * ((double)elapsedMs / 3600000.0);
  if (baseline < 1e-6) return 0.0;
  double frac = lampEnergyWh / baseline;
  if (frac < 0.0) frac = 0.0;
  if (frac > 1.0) frac = 1.0;
  const double u = sqrt(lampEnergyErrorPct * lampEnergyErrorPct +
                        baselineErrorPct * baselineErrorPct);
  return frac * u;
}

// --- Dashboard token auth ----------------------------------------------------

// Format a 48-bit MAC as 12 lowercase hex digits + NUL.
static inline bool formatMacToken(uint64_t mac, char *out, size_t outLen) {
  if (!out || outLen < 13u) return false;
  static const char kHex[] = "0123456789abcdef";
  for (int i = 11; i >= 0; --i) {
    out[i] = kHex[mac & 0xFu];
    mac >>= 4;
  }
  out[12] = '\0';
  return true;
}

// Constant-time, NULL-safe token comparison.
static inline bool tokenEquals(const char *a, const char *b) {
  if (!a || !b) return false;
  size_t ia = 0, ib = 0;
  unsigned char diff = 0;
  for (;;) {
    const char ca = a[ia], cb = b[ib];
    diff |= (unsigned char)(ca ^ cb);
    if (ca == '\0' && cb == '\0') break;
    if (ca != '\0') ++ia;
    if (cb != '\0') ++ib;
  }
  return diff == 0;
}

// Cookie values use only letters, digits, '_', '-' and '.'. Length 1..maxLen.
static inline bool validCookieValue(const char *value, size_t maxLen) {
  if (!value || maxLen == 0) return false;
  size_t n = 0;
  for (const char *p = value; *p; ++p) {
    const unsigned char c = (unsigned char)*p;
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    if (!ok) return false;
    if (++n > maxLen) return false;
  }
  return n >= 1;
}

// Extract cookie `name` from a Cookie header into out. Exact key match.
static inline bool findCookieValue(const char *header, const char *name,
                                   char *out, size_t outLen) {
  if (!header || !name || !out || outLen == 0) return false;
  size_t nameLen = 0;
  while (name[nameLen]) ++nameLen;
  if (nameLen == 0) return false;
  const char *p = header;
  while (*p) {
    while (*p == ' ' || *p == '\t' || *p == ';') ++p;
    if (!*p) break;
    const char *key = p;
    while (*p && *p != '=' && *p != ';') ++p;
    size_t keyLen = (size_t)(p - key);
    while (keyLen > 0 && key[keyLen - 1] == ' ') --keyLen;
    const char *value = NULL;
    if (*p == '=') {
      ++p;
      while (*p == ' ' || *p == '\t') ++p;
      value = p;
      while (*p && *p != ';') ++p;
    }
    if (value && keyLen == nameLen) {
      bool match = true;
      for (size_t i = 0; i < nameLen; ++i) {
        if (key[i] != name[i]) { match = false; break; }
      }
      if (match) {
        size_t valueLen = (size_t)(p - value);
        while (valueLen > 0 && (value[valueLen - 1] == ' ' ||
                                value[valueLen - 1] == '\t' ||
                                value[valueLen - 1] == '\r')) --valueLen;
        if (valueLen == 0 || valueLen >= outLen) return false;
        for (size_t i = 0; i < valueLen; ++i) out[i] = value[i];
        out[valueLen] = '\0';
        return true;
      }
    }
    if (*p == ';') ++p;
  }
  return false;
}

// --- R6: MQTT + Home Assistant discovery (pure helpers) ----------------------
// Tidak bergantung pada Arduino/WiFi/PubSubClient supaya bisa diuji di host.
// Glue board (koneksi, publish, reconnect) ada di mqtt.h.

static inline bool mqttConfigured(const char *broker) {
  if (!broker || !*broker) return false;
  return strcmp(broker, "GANTI_BROKER") != 0;
}

// Enam digit heksadesimal dari 24 bit bawah MAC. Empat digit punya ~0,7%
// peluang tabrakan untuk 30 perangkat, jadi pakai enam.
static inline bool mqttDeviceId(uint64_t mac, char *out, size_t n) {
  if (!out || n < 7u) return false;
  static const char kHex[] = "0123456789abcdef";
  for (int i = 5; i >= 0; --i) {
    out[i] = kHex[(unsigned)(mac & 0xFu)];
    mac >>= 4;
  }
  out[6] = '\0';
  return true;
}

enum MqttTopicKind {
  SLX_TOPIC_BASE = 0,
  SLX_TOPIC_STATE = 1,
  SLX_TOPIC_AVAIL = 2,
  SLX_TOPIC_DISCOVERY = 3,
};

// Prefix bisa dioverride lewat secrets.h; default sesuai rekomendasi HA.
static inline bool mqttTopic(int kind, const char *id, const char *component,
                             const char *object, char *out, size_t n,
                             const char *baseTopic = "smartlux",
                             const char *discoveryPrefix = "homeassistant") {
  if (!id || !out || n == 0 || !baseTopic || !discoveryPrefix) return false;
  int w;
  switch (kind) {
    case SLX_TOPIC_BASE:
      w = snprintf(out, n, "%s/%s", baseTopic, id);
      break;
    case SLX_TOPIC_STATE:
      w = snprintf(out, n, "%s/%s/state", baseTopic, id);
      break;
    case SLX_TOPIC_AVAIL:
      w = snprintf(out, n, "%s/%s/availability", baseTopic, id);
      break;
    case SLX_TOPIC_DISCOVERY:
      if (!component || !object) return false;
      w = snprintf(out, n, "%s/%s/smartlux_%s/%s/config",
                   discoveryPrefix, component, id, object);
      break;
    default:
      return false;
  }
  return w > 0 && (size_t)w < n;
}

// millis() ESP32 32-bit. Pakai uint32_t (bukan unsigned long: 64-bit di host)
// supaya perilaku rollover di tes host sama dengan di board.
static inline bool mqttShouldAttempt(uint32_t nowMs, uint32_t lastMs,
                                     uint32_t delayMs) {
  return (nowMs - lastMs) >= delayMs;
}

// Backoff 5/5/10/20/30 s, dibatasi supaya shift tidak overflow.
static inline uint32_t mqttNextBackoffMs(unsigned int attempt) {
  if (attempt == 0) return 5000UL;
  if (attempt >= 4) return 30000UL;
  return 5000UL << (attempt - 1);
}

struct MqttEntity {
  const char *component;
  const char *object;
  const char *name;
  const char *valueTemplate;
  const char *deviceClass;    // nullptr bila tidak ada
  const char *stateClass;     // nullptr bila tidak ada
  const char *unit;           // nullptr bila tidak ada
  const char *icon;           // nullptr bila tidak ada
  const char *entityCategory; // nullptr bila tidak ada
  const char *options;        // array JSON, hanya untuk device_class enum
  bool binary;
};

// Tabel static lokal fungsi: hanya di-emit di TU yang memanggilnya. Build dasar
// (tanpa mqtt.h) tidak menyertakan rodata ini, jadi byte-for-byte tidak berubah.
static inline const MqttEntity *mqttEntities(size_t *count) {
  static const MqttEntity kEntities[] = {
    {"binary_sensor", "motion", "Gerakan",
     "{{ value_json.pir == 1 }}", "motion", nullptr, nullptr,
     "mdi:motion-sensor", nullptr, nullptr, true},
    {"sensor", "illuminance", "Iluminasi (estimasi)",
     "{{ value_json.lux_est }}", "illuminance", nullptr, "lx",
     "mdi:brightness-6", nullptr, nullptr, false},
    {"binary_sensor", "lamp", "Lampu",
     "{{ value_json.lamp == 1 }}", "light", nullptr, nullptr,
     "mdi:lightbulb", nullptr, nullptr, true},
    {"sensor", "power", "Daya",
     "{{ value_json.power_mw / 1000 }}", "power", nullptr, "W",
     "mdi:flash", nullptr, nullptr, false},
    {"sensor", "energy", "Energi Sistem",
     "{{ value_json.energy_wh }}", "energy", "total_increasing", "Wh",
     "mdi:chart-line", nullptr, nullptr, false},
    {"sensor", "savings", "Penghematan (vs selalu nyala)",
     "{{ value_json.savings_pct }}", nullptr, nullptr, "%",
     "mdi:leaf", nullptr, nullptr, false},
    {"sensor", "mode", "Mode",
     "{{ value_json.mode }}", "enum", nullptr, nullptr,
     "mdi:auto-mode", nullptr, "[\"auto\",\"on\",\"off\"]", false},
    {"binary_sensor", "override", "Override",
     "{{ value_json.mode != 'auto' }}", nullptr, nullptr, nullptr,
     "mdi:hand-back-right", nullptr, nullptr, true},
    {"sensor", "pwm", "PWM Lampu",
     "{{ value_json.lamp_pwm / 255 * 100 }}", nullptr, nullptr, "%",
     "mdi:pulse", "diagnostic", nullptr, false},
  };
  if (count) *count = sizeof(kEntities) / sizeof(kEntities[0]);
  return kEntities;
}

static inline bool slxAppendf(char *out, size_t n, size_t &pos,
                              const char *fmt, ...) {
  if (!out || !fmt || pos >= n) return false;
  va_list ap;
  va_start(ap, fmt);
  const int w = vsnprintf(out + pos, n - pos, fmt, ap);
  va_end(ap);
  if (w < 0 || (size_t)w >= n - pos) return false;
  pos += (size_t)w;
  return true;
}

// Susun payload config discovery yang dibatasi `n`; -1 bila terpotong.
static inline int buildMqttDiscoveryConfig(const MqttEntity *e, const char *id,
                                           char *out, size_t n,
                                           const char *baseTopic = "smartlux") {
  if (!e || !id || !out || n == 0 || !baseTopic) return -1;
  size_t pos = 0;
  bool ok = slxAppendf(out, n, pos,
      "{\"name\":\"%s\",\"unique_id\":\"smartlux_%s_%s\","
      "\"~\":\"%s/%s\",\"state_topic\":\"~/state\","
      "\"availability_topic\":\"~/availability\",\"expire_after\":60,"
      "\"json_attributes_topic\":\"~/state\"",
      e->name, id, e->object, baseTopic, id);
  if (e->deviceClass)
    ok = ok && slxAppendf(out, n, pos, ",\"device_class\":\"%s\"", e->deviceClass);
  if (e->stateClass)
    ok = ok && slxAppendf(out, n, pos, ",\"state_class\":\"%s\"", e->stateClass);
  if (e->unit)
    ok = ok && slxAppendf(out, n, pos, ",\"unit_of_measurement\":\"%s\"", e->unit);
  if (e->icon)
    ok = ok && slxAppendf(out, n, pos, ",\"icon\":\"%s\"", e->icon);
  if (e->entityCategory)
    ok = ok && slxAppendf(out, n, pos, ",\"entity_category\":\"%s\"", e->entityCategory);
  if (e->options)
    ok = ok && slxAppendf(out, n, pos, ",\"options\":%s", e->options);
  if (e->binary)
    ok = ok && slxAppendf(out, n, pos,
                          ",\"payload_on\":\"True\",\"payload_off\":\"False\"");
  ok = ok && slxAppendf(out, n, pos,
      ",\"device\":{\"identifiers\":[\"smartlux_%s\"],"
      "\"name\":\"SMARTLUX %s\",\"manufacturer\":\"SMAN CMBBS\","
      "\"model\":\"ESP32 DevKit\",\"sw_version\":\"2.0\","
      "\"configuration_url\":\"http://smartlux.local/\"},"
      "\"origin\":{\"name\":\"SMARTLUX firmware\",\"sw\":\"2.0\","
      "\"url\":\"https://github.com/FlintsXzzz/smartlux\"}}",
      id, id);
  return ok ? (int)pos : -1;
}
