// logic.h - pure, host-testable logic for SMARTLUX.
//
// No Arduino dependencies: this header is included by SMARTLUX.ino (so the
// board and the host run the same code) and by tests/native (so the rules are
// covered by unit tests without hardware).
#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>
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
