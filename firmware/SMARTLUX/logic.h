// logic.h - pure, host-testable logic for SMARTLUX.
//
// No Arduino dependencies: this header is included by SMARTLUX.ino (so the
// board and the host run the same code) and by tests/native (so the rules are
// covered by unit tests without hardware).
#pragma once

#include <math.h>
#include <stdint.h>
#include <string.h>

// Mode encoding matches `enum Mode` in SMARTLUX.ino:
//   AUTO = 0, MANUAL_ON = 1, MANUAL_OFF = 2
// Returns whether the lamp should be ON for the given sensors and mode.
static inline bool lampShouldBeOn(int mode, bool pirReady, bool dark, bool motion) {
  if (!pirReady) return false;  // PIR warm-up: force a safe OFF
  if (mode == 1) return true;   // MANUAL_ON ignores the sensors
  if (mode == 2) return false;  // MANUAL_OFF ignores the sensors
  return dark && motion;        // AUTO: dark AND motion (delay-off handled by caller)
}

// Next mode when the override button cycles: AUTO -> ON -> OFF -> AUTO.
static inline int cycleModeValue(int mode) { return (mode + 1) % 3; }

// Parse a /api/override `mode` argument. Returns 0/1/2, or -1 when invalid.
static inline int parseMode(const char *m) {
  if (!m) return -1;
  if (!strcmp(m, "auto")) return 0;
  if (!strcmp(m, "on")) return 1;
  if (!strcmp(m, "off")) return 2;
  return -1;
}

// JSON/log key for a mode: "auto" | "on" | "off".
static inline const char *modeKeyFor(int mode) {
  if (mode == 1) return "on";
  if (mode == 2) return "off";
  return "auto";
}

// LDR hysteresis: -1 = keep previous state, 0 = dark, 1 = bright.
static inline int lightDecision(int adc, int darkOn, int brightOff) {
  if (adc < darkOn) return 0;
  if (adc > brightOff) return 1;
  return -1;
}

// Map a 12-bit ADC reading to 0..100 %, clamped.
static inline int lightPercent(int adc) {
  if (adc < 0) adc = 0;
  if (adc > 4095) adc = 4095;
  return (int)((long)adc * 100L / 4095L);
}

// Rough lux estimate for the non-linear LDR; full scale at ADC 4095.
static inline float luxEstimate(int adc, float fullScale) {
  const float ratio = (float)adc / 4095.0f;
  return fullScale * powf(ratio, 2.2f);
}

// True while the delay-off window since the last motion is still open.
static inline bool motionActive(unsigned long now, unsigned long lastMotionMs,
                                unsigned long delayMs) {
  return (now - lastMotionMs) < delayMs;
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
