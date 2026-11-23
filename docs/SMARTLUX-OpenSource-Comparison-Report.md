> Source: TinyFish deep research run [01a2083e-cab8-7e7f-bac4-be00dabbe42b](https://agent.tinyfish.ai/research/01a2083e-cab8-7e7f-bac4-be00dabbe42b) (mode: deep, completed 2026-11-23, 48 sources read).

# SMARTLUX Open-Source Comparison Report
## Cited Analysis & Prioritized Adoption Recommendations

**Prepared for:** SMAN CMBBS Kelompok 2, 2026/2027
**Date:** 23 November 2026
**Scope:** ESP32/Arduino PIR+LDR smart-lamp ecosystem survey; feature comparison; ranked firmware adoption roadmap

---

## Executive Summary

SMARTLUX is a genuinely well-engineered high-school practicum project that already surpasses virtually every comparable open-source project found in the community. Its combination of INA219 real-power metering with savings accounting, explicit 11-state FSM, 20/20 native unit tests, GitHub Actions CI, rate-limited token-authenticated dashboard, hysteresis darkness logic, and calibrated ADC is rare — most direct look-alikes are weekend sketches with zero tests and no energy measurement. The ecosystem leaders (WLED LDR_PIR usermod, ESPHome, Tasmota) offer specific practices worth borrowing: MQTT+Home Assistant auto-discovery, ADC median filtering, software watchdog feeding, NVS-persisted energy accumulator, and in-browser history charts. PWM dimming via the existing MOSFET is achievable with zero new hardware. OTA updates are desirable but constrained by the 78% flash budget. A ranked, hardware-constrained adoption list is provided below.

---

## Part I — Direct Look-Alike Projects (Tier 1)

### 1.1 hari2k7/smart-lighting-system

**URL:** https://github.com/hari2k7/smart-lighting-system
**Stars/activity:** 17 stars, 1 fork, 4 commits, last updated February 2026 [1][10]
**Hardware:** ESP32, PIR motion sensor, LDR module, potentiometer, three LEDs [1]
**Feature set:** Lights turn ON when motion is detected under low ambient light; potentiometer adjusts sensitivity threshold; Wokwi-simulator-only validation, no real-hardware testing [1]. No WiFi, no dashboard, no energy metering, no OLED, no state machine, no tests, no CI [1].
**Code architecture/quality:** Single `sketch.ino` + `diagram.json` + `README.md`, beginner-level, no abstraction [1].

**vs. SMARTLUX:** This is the closest conceptual match (same PIR + LDR on ESP32), but SMARTLUX is architecturally superior in every measurable dimension — INA219 power metering, hysteresis-based darkness threshold, 11-state FSM, unit tests, CI, authenticated web dashboard, OLED, WiFi/mDNS, OpenSCAD enclosure. hari2k7's project has no network connectivity, no energy accounting, no override logic. Its only marginal advantage is a live Wokwi diagram for in-browser simulation, which SMARTLUX lacks.

---

### 1.2 Botunade/MotionLights

**URL:** https://github.com/Botunade/MotionLights
**Stars/activity:** 0 stars, 0 forks [5]
**Hardware:** ESP32, dual PIR sensors on GPIO34/35, four relay outputs on GPIO16–19, pushbuttons on GPIO13/14/26/27 [5]
**Feature set:** 4-channel motion + manual override lighting controller; dual PIR (logical OR); hardware-interrupt debounce (50 ms); automatic override reset on room vacancy; channel isolation; PlatformIO build; modular config.h / main.cpp architecture [5].
**Code quality:** FreeRTOS critical sections (portENTER_CRITICAL) protecting ISR-shared state; parametric config header; no energy monitoring, no LDR, no dashboard, no WiFi, no MQTT, no OTA, no tests, no CI [5].

**vs. SMARTLUX:** SMARTLUX has LDR darkness gating that MotionLights entirely lacks (always-on when motion). MotionLights' critical-section ISR pattern and modular config.h separation are cleaner than many peers but SMARTLUX's logic.h extraction with 20/20 native tests is more rigorous. MotionLights offers a 4-channel relay model and dual-PIR OR logic worth noting for multi-room extensions but irrelevant to SMARTLUX's single-lamp school scope.

---

### 1.3 GridLessTech/solar-bottle-lights-monitor

**URL:** https://github.com/GridLessTech/solar-bottle-lights-monitor
**Stars/activity:** 1 star, 0 forks [6]
**Hardware:** ESP32/Arduino, INA219 current sensor, MQTT broker, HTML/CSS/JS web dashboard [6]
**Feature set:** Monitors voltage, current, power and energy in real time; remote light control via web dashboard; MQTT-based communication [6]. No PIR, no LDR, no occupancy logic, no darkness gating [6].
**Code quality:** No described tests, CI, watchdog, sensor calibration, or deep sleep [6].

**vs. SMARTLUX:** Shares INA219 + MQTT + web dashboard intent but is not a PIR/LDR smart lamp. SMARTLUX adds occupancy+darkness logic, state machines, savings math, unit tests, and CSP-secured token auth that this project lacks entirely. The key borrowable practice is **MQTT publishing of INA219 readings**, which SMARTLUX currently lacks.

---

### 1.4 Rggitty/edge-ai-security-camera-node

**URL:** https://github.com/Rggitty/edge-ai-security-camera-node
**Stars/activity:** 0 stars, 0 forks [7]
**Hardware:** ESP32-CAM (SunFounder), PIR on GPIO4, OV2640 camera, INA219 on GPIO13 (SDA)/GPIO14 (SCL) powered from 3.3V [7]
**Feature set:** PIR-triggered camera capture; Edge Impulse person/no_person TFLite classifier; INA219 inline power profiling between power bank and ESP32 USB-C input; WiFi dashboard with AI frame; CSV event logging; active-dashboard vs. WiFi-off power comparison mode [7].
**Code quality:** No MQTT, no HA discovery, no OTA, no watchdog, no unit tests, no CI [7].

**vs. SMARTLUX:** Shares PIR + INA219 + WiFi dashboard combination. SMARTLUX's INA219 integration is more pedagogically correct (high-side on 5 V rail with savings math) versus this project's USB-C inline profiling. SMARTLUX's energy accounting and state machine architecture are more complete. What this project adds over SMARTLUX is **Wi-Fi-off power comparison mode** (measuring controller overhead vs. lamp) and CSV export — both are worth considering.

---

### 1.5 Arlene-Riona/IoT-Smart-Lighting

**URL:** https://github.com/Arlene-Riona/IoT-Smart-Lighting
**Stars/activity:** 0 stars, 0 forks [11]
**Hardware:** Arduino Uno, PIR on D2, LDR on A0, LED on PWM pin D9 [11]
**Feature set:** PIR decides when to turn on; LDR decides how bright (brightness = 255 − scaled LDR); daylight harvesting via PWM; Python serial dashboard showing natural light, brightness, occupancy, cumulative LED-on seconds [11].
**Code quality:** No ESP32, no network, no MQTT, no CI, no watchdog [11].

**vs. SMARTLUX:** SMARTLUX lacks PWM dimming (pure on/off) and the concept of **daylight-harvesting brightness proportional to darkness** is a concrete gap. This project's Python dashboard showing cumulative on-seconds is a rudimentary energy proxy. SMARTLUX's INA219 actual watt-hour accounting is far superior to LED-on-seconds estimation.

---

### 1.6 jkskw/smart_socket

**URL:** https://github.com/jkskw/smart_socket
**Stars/activity:** 4 stars, 0 forks [25]
**Hardware:** ESP32, SRD-05VDC relay, ACS712-20A current sensor, ZMPT101B voltage sensor, DHT11, HC-SR501 PIR, DS1307 RTC, Hi-Link HLK-PM01 5VDC PSU [25]
**Feature set:** "Night Light Mode" activates socket on PIR motion with configurable duration; real-time energy monitoring (current, voltage, active power, total energy); weekday/hour scheduling via RTC; temperature mode; overload/thermal safety with alarm logging; web UI with AP-mode setup; default admin/admin credentials (changeable) [25].
**Code quality:** No MQTT, no HA discovery, no OTA described; mains-powered hardware [25].

**vs. SMARTLUX:** Night Light Mode is conceptually similar but lacks LDR darkness gating. The **alarm logging with timestamp and pre-alarm measurement** is a practice worth noting. The scheduling feature (weekday/hour via RTC) is a useful extension. However, its mains-connected ACS712/ZMPT101B sensor approach violates SMARTLUX's hard constraint (single regulated 5 V, never mains); this remains a stretch-goal-only reference.

---

### 1.7 Arjunren/SmartHomeLightingSystem *(GitHub topic listing)*

**URL:** Listed at https://github.com/topics/smart-home-lighting [9]
**Stars/activity:** 0 stars, updated October 2026 [9]
**Hardware:** ESP32, LDR voltage divider, PIR motion sensor, opto-isolated low-voltage relay [9]
**Feature set:** Measures light level, publishes validated telemetry over WiFi and TLS MQTT; wiring, security, calibration, source code, sensor checks, sample data documented [9].

**vs. SMARTLUX:** Closest conceptual twin including TLS MQTT. SMARTLUX lacks MQTT entirely. The TLS MQTT pattern (pub over WiFi with validated telemetry) is exactly what SMARTLUX should consider as an optional layer.

---

### 1.8 Academic/IoT PIR+ACS712 Corridor Lighting (JEESR paper)

**Source:** Journal of Electrical Engineering Systems Research, Vol. 29 [48]
**Hardware:** ESP32, PIR, 3× relay, 3× ACS712, 3× 9W LED lamps, Blynk, Google Apps Script, Google Sheets [48]
**Feature set:** PIR relay control; 30 s inactivity timeout; per-channel ACS712 fault detection with hysteresis thresholds (0.075–0.092 A with recovery at 0.081–0.098 A); energy estimated from rated power × ON intervals (not measured); claimed 97.87% saving vs. theoretical always-on [48].
**Code quality:** Relay on/off only, no PWM; energy estimated not measured; no OTA, no MQTT, no tests [48].

**vs. SMARTLUX:** SMARTLUX's INA219 actual power measurement is methodologically superior to ACS712 current estimation from rated wattage [48]. SMARTLUX's savings math is more honest. The per-channel **fault detection with hysteresis** (double threshold for recovery) is a noteworthy robustness pattern. The paper explicitly warns that the 97.87% saving is a theoretical upper bound and should not be interpreted as deployment expectation [48] — SMARTLUX's 10–40% honest estimate is more credible.

---

### 1.9 Other Minor Look-Alikes (brief)

| Project | Repo | Stars | Key Differentiator vs. SMARTLUX |
|---|---|---|---|
| KeerthiYarashi/Smart-Street-Lighting | [2] | 1 | Blynk dashboard; LDR day/night + IR motion; adaptive dim (DIM/BRIGHT); no INA219 |
| imanishraj/Smart-Street-Light-System | [12] | 0 | BH1750 digital lux vs. LDR ADC; stepped PWM brightness levels; no ESP32, no WiFi |
| GR4Yxx/AutomaticCollegeLights-Arduino | [22] | 0 | PIR + LDR + Bluetooth master-slave; no WiFi, no energy metering |
| SumaitaB/IlluminateTech | [33] | 0 | Arduino+NodeMCU split; Blynk remote; LDR streetlight; no INA219 |
| pranjal-joshi/Radiance | [34] | 0 | ESP8266 PIR smart lamp; Blynk + Google Assistant; no LDR darkness gating |
| egalistel-dev/lumisetup-esp8266 | [35] | 3 | ESP8266 PIR+WS2812B; NTP time-range gating; LittleFS config; no INA219, no LDR |
| ayush-gangwar-09/Smart-Street-Light-IoT | [39] | 1 | ESP32 + LDR + ThingsBoard; no PIR yet (listed as future); no INA219 |
| jackshiels/LightSense | [13] | 1 | UCL CASA0016; PIR + light sensor; MQTT to Raspberry Pi; no actuator output |
| prakashkarkee/smartclass | [21] | 1 | University of Oulu; ESP32 PIR+DHT11+mic; Firebase; historical charts; no LDR lux |
| pedrogbmendes/SCDTR-Project | [23] | 1 | Distributed LDR-only cooperative illuminance control; no PIR, no WiFi dashboard |
| parikshitpagare/smart-home-automation-rtos | [14] | 100 | ESP32 + FreeRTOS + LDR + Bluetooth; most popular look-alike; no PIR for lights, no INA219 |

---

## Part II — Ecosystem Reference Projects (Tier 2)

### 2.1 WLED LDR_PIR Usermod

**URLs:** [3][36][17]
**Repository:** wled/WLED, `usermods/LDR_PIR/`

WLED's LDR_PIR usermod is the most direct ecosystem reference: it combines LDR (single analogRead normalized to 0–1) and PIR (digitalRead INPUT_PULLUP) to activate a preset only when dark AND motion detected, with a configurable off-delay (default 60 s) and dual thresholds creating hysteresis [3][36]. It is polled every 250 ms with no median filtering — a known weakness [36][17]. It publishes motion and light state over MQTT and supports optional Home Assistant MQTT auto-discovery for motion (device_class: motion) and illuminance sensors [3][36]. Its discovery JSON is built in a `StaticJsonDocument<600>` buffer [36]. There is a reported undefined-variable bug (`offMode`) in the PR that may prevent compilation [17]. No energy logging, no deep sleep, no PWM dimming [36]. SMARTLUX already matches or exceeds this usermod in every hardware-correctness and software-quality dimension; the borrowable practices are **MQTT pub format** and **HA auto-discovery pattern**.

### 2.2 WLED INA219 Usermod

**URL:** [4]
**Repository:** wled/WLED, `usermods/INA219/`

Monitors voltage, current, power; accumulates daily/monthly/total energy; resets daily after 24 h, monthly after 30 days [4]. Configurable ADC conversion time from 9-bit (84 µs) to 128-sample averaging (68.10 ms) [4]. Configurable correction factor and shunt resistor value for calibration [4]. Supports MQTT pub and HA auto-discovery [4]. Check interval default 5 s, configurable; can publish only on change [4]. This directly maps onto SMARTLUX's existing INA219 integration but with **daily/monthly energy bucketing and configurable ADC averaging** that SMARTLUX currently lacks.

### 2.3 WLED PIRsensorSwitch Usermod

**URL:** [24]
**Repository:** wled/WLED MoonModules, `usermods/PIRsensorSwitch`

PIR on GPIO13; 10-minute default off-delay; nighttime-only triggering via NTP + lat/lon; MQTT `wled/<MAC>/motion` topic; HA autodiscovery added in 2022-11 [24]. Web info page shows remaining off-timer and allows temporarily disabling PIR [24]. Exposes `PIRsensorEnabled()` API so other usermods can query state [24]. Key practice: **exposing remaining off-timer on the dashboard** and **nighttime-only restriction via NTP** are both applicable to SMARTLUX.

### 2.4 ESPHome Binary Sensor + PIR + BH1750 Patterns

**URLs:** [31][18]

ESPHome's binary sensor component provides `delayed_on_off` and `timeout` filters for PIR debouncing and stale-reading invalidation [31]. The EverythingSmartHome everything-presence-one config combines mmWave + PIR with a template lambda combining both into a composite occupancy binary sensor, with user-adjustable off-latency stored with `restore_value` [18]. BH1750 illuminance with 30 s update interval and user-adjustable offset calibration entities [18]. OTA, API, captive portal, and GitHub-based dashboard import all built into ESPHome [18]. Key practice: **composite occupancy with restore_value** and **runtime-adjustable sensor calibration offset** are relevant to SMARTLUX.

### 2.5 ESPHome Median Filter

**URL:** [41]

ESPHome's median filter over a configurable window (default 5, odd number recommended) removes ADC outliers without introducing latency proportional to a running average [41]. `send_every` and `send_first_at` control publish frequency [41]. This is directly applicable to SMARTLUX's LDR ADC on GPIO34.

### 2.6 Tasmota PIR Handling and Power Calibration

**URLs:** [30][32]

Tasmota maps PIR as a Switch and publishes motion events via MQTT rules [30]. HA discovery is published as `homeassistant/binary_sensor/<topic>/config` retained [30]. HC-SR501 configured with `SwitchMode 1` leverages the sensor's internal retrigger and is the same module SMARTLUX uses [30]. Power calibration uses `VoltageSet`, `PowerSet`, `CurrentCal`/`PowerCal`/`VoltageCal` up to ±32000 offset; calibration must use a resistive (not LED) load because LED drivers introduce power-factor errors [32]. This confirms SMARTLUX should **document that its INA219 calibration baseline should use a pure resistive load** for energy-savings accuracy, not the LED lamp directly.

### 2.7 Home Assistant Blueprint: Motion + Illuminance + Two-Level Dimming

**URL:** [29]

HA blueprint by EdwardTFN implements: motion-activated light when illuminance < configurable lux threshold (default 15 lx) AND sun elevation condition; two-level dimming (high brightness → dim → off) with configurable timeouts; bathroom/closed-door hold logic; pause entities for party/TV scenarios [29]. Although HA-side only (not on-device), the **two-level dimming pattern** (bright on motion, dim before timeout, off) is directly adaptable to SMARTLUX's MOSFET PWM via LEDC. The illuminance-triggered blueprint uses the same logic SMARTLUX already implements on-device, confirming the design is sound.

### 2.8 EcoSentinel (Atlas-did)

**URL:** [16]

ESP32-S3 + INA219 + relays + React dashboard; energy saving by integrating INA219 power over time into Wh/kWh versus a baseline [16]. Implements anti-short-cycle protection (minimum 15-minute run, 10-minute stop), ±0.5°C hysteresis deadband, stale-data detection → safe mode, I2C bus recovery, MCU soft reset with cooldown [16]. MQTT client in hardware layer, FastAPI backend, React frontend [16]. pytest suite [16]. The **I2C bus recovery** and **stale-data detection → safe mode** patterns are directly applicable to SMARTLUX's INA219 and SSD1306 on the same I2C bus.

### 2.9 UG56-Lab-PSU (universalgeek56)

**URL:** [27]

ESP32 with PWM+MOSFET+RC-filter feedback; PID CV/CC regulation; NVS via `PreferencesManager`; OTA over WiFi; WebSocket dashboard with live V/I/P/TEMP charts; 18 safety flags; INA226 sensing [27]. Key practices: **NVS Preferences for persistent settings**, **WebSocket for live charts** (vs. polling), and **OTA over WiFi** are all relevant to SMARTLUX.

### 2.10 ESP32-Terrarium (Rau7han)

**URL:** [28]

ESP32 + DHT11 + SSD1306 + relay + OTA (ArduinoOTA); REST/JSON API (`/api/live`, `/api/history`); in-browser 100-point history chart (1-min resolution); OLED cycles screens every 4 s; short/long-press button behavior analogous to SMARTLUX's override button; persistent settings in flash [28]. Key practices: **100-point in-browser history chart** and **per-minute ring buffer stored in RAM** are low-cost additions to SMARTLUX's dashboard.

---

## Part III — Feature Comparison Table

| Feature | SMARTLUX | hari2k7 [1] | MotionLights [5] | WLED LDR_PIR [3][36] | ESPHome EPO [18] | Tasmota [30] | GridLessTech [6] | jkskw smart_socket [25] | ESP32-Terrarium [28] | EcoSentinel [16] |
|---|---|---|---|---|---|---|---|---|---|---|
| **PIR motion sensing** | ✅ HC-SR501, 60s warmup, retrigger | ✅ | ✅ Dual PIR | ✅ | ✅ PIR+mmWave | ✅ | ❌ | ✅ HC-SR501 | ❌ | ❌ |
| **LDR ambient light** | ✅ Calibrated, 10-20% hysteresis | ✅ Potentiometer adj | ❌ | ✅ Dual threshold hysteresis | ✅ BH1750 digital | ❌ native | ❌ | ❌ | ❌ | ❌ |
| **Dark AND motion logic** | ✅ | ✅ | ❌ | ✅ | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **ADC filtering/calibration** | ✅ 11dB, 100nF bypass, empirical | ❌ | ❌ | ❌ Single read | ✅ Offset cal | ❌ | ❌ | ❌ | ❌ | ❌ |
| **PWM dimming** | ❌ On/off only | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **INA219 real energy meter** | ✅ 0.1Ω shunt, E_Wh integrated | ❌ | ❌ | ✅ (separate INA219 usermod) [4] | ❌ | ❌ | ✅ INA219 | ❌ ACS712 | ❌ | ✅ INA219 |
| **Savings vs baseline** | ✅ Explicit math | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ (retracted sim) [16] |
| **State machine** | ✅ 11 explicit enum states | ❌ | Partial (channel isolation) | ❌ | ESPHome automations | ❌ | ❌ | ❌ | ❌ | Partial |
| **Override button** | ✅ Short/long press, 3-mode cycle | ❌ | ✅ Hardware ISR | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ Short/2s long | ❌ |
| **OLED display** | ✅ SSD1306 128×64 status pages | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ SSD1306 | ❌ |
| **WiFi dashboard** | ✅ PROGMEM, dark theme, 23 JSON keys | ❌ | ❌ | ✅ WLED UI | ✅ HA | ✅ HA | ✅ | ✅ | ✅ /api/history | ✅ React |
| **In-browser charts/history** | ❌ | ❌ | ❌ | ❌ | ✅ via HA | ❌ | ❌ | ❌ | ✅ 100 pts | ✅ React |
| **mDNS** | ✅ smartlux.local | ❌ | ❌ | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ terrarium-esp32 | ❌ |
| **AP fallback** | ✅ SMARTLUX-XXXX | ❌ | ❌ | ✅ | ✅ captive portal | ✅ | ❌ | ✅ | ✅ | ❌ |
| **CSP / security headers** | ✅ CSP default-src 'none' | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Session token auth** | ✅ secrets.h / MAC-derived | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ admin/admin | ❌ | ❌ |
| **Rate limiting (429)** | ✅ 1 action/300ms | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **MQTT** | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ | ✅ |
| **HA auto-discovery** | ❌ | ❌ | ❌ | ✅ [3][36] | ✅ | ✅ | ❌ | ❌ | ❌ partial | ❌ |
| **OTA updates** | ❌ | ❌ | ❌ | ✅ WLED | ✅ ESPHome | ✅ | ❌ | ❌ | ✅ ArduinoOTA | ❌ |
| **NVS persistent energy** | ❌ (RAM only) | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ (settings) | ❌ |
| **Unit tests** | ✅ 20/20 Unity host tests | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ pytest |
| **CI pipeline** | ✅ GitHub Actions | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Watchdog (TWDT)** | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ soft reset |
| **3D enclosure** | ✅ OpenSCAD parametric | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **SECURITY.md** | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Accessibility reviewed** | ✅ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ |
| **Licence** | MIT | ❌ stated | MIT [5] | GPL | Apache | GPL | ❌ stated | MIT [25] | ❌ stated | ❌ stated |

**Legend:** ✅ = implemented; ❌ = absent; "partial" = limited implementation

---

## Part IV — Per-Project Key Differences Summary

**Where SMARTLUX leads across the board:**
- Real INA219 watt-hour metering with explicit savings vs. always-on baseline (most projects estimate from rated watts or omit entirely) [1][48]
- 11-state explicit FSM with observability keys (others use ad-hoc if/else) [5]
- Native Unity host tests 20/20 + GitHub Actions CI (zero competing projects have this) [38]
- CSP `default-src 'none'` + session-cookie token auth + rate limiting (no comparable project has all three) [25]
- Calibrated ADC with 11 dB attenuation, 100 nF bypass capacitor, empirical hysteresis [1][36]
- OpenSCAD parametric enclosure [none]
- Accessibility-reviewed dashboard [none]
- SECURITY.md [none]

**Where SMARTLUX has gaps vs. the ecosystem:**
- **No MQTT / HA auto-discovery** — WLED LDR_PIR, ESPHome, Tasmota all publish motion and light state to MQTT and auto-register with Home Assistant [3][30][36]
- **No OTA updates** — WLED, ESPHome, Tasmota, lumisetup, terrarium all support OTA [28][43]
- **No PWM dimming** — hari2k7, IoT-Smart-Lighting, BH1750-based projects all implement stepless or stepped dimming [11][12]
- **No persistent energy accumulator** — energy E_Wh resets on power loss; WLED INA219 usermod retains daily/monthly totals [4]
- **No in-browser history charts** — WLED INA219 usermod, EcoSentinel, ESP32-Terrarium, smartclass all show historical plots [16][21][28]
- **No ADC median filtering** — WLED LDR_PIR takes single reads [36]; ESPHome uses explicit median filter [41]
- **No software watchdog feed** — EcoSentinel implements I2C bus recovery + MCU soft reset [16]; ESP-IDF TWDT is not subscribed [44]

---

## Part V — Ranked Adoption List with Implementation Notes

Priorities are ordered by **value/effort ratio** within SMARTLUX's constraints (5V single supply, existing pins, ~8.1 KB PROGMEM budget, 78% flash). Items marked 🔧 require no new hardware; items marked ⚙️ are stretch goals needing extra hardware.

---

### R1 🔧 ADC Median Filtering on GPIO34 LDR (Value: HIGH / Effort: LOW)

**Why:** ESP32 ADC1 is well-known for noise, especially with WiFi radio active. WLED LDR_PIR takes a single `analogRead` and explicitly has no filtering [36]. ESPHome's median filter removes outliers in a 5-sample window [41]. SMARTLUX's existing 100 nF bypass is good hardware practice, but a software 5-sample median eliminates ADC glitches that could cause false lamp activations.

**Implementation (firmware):**
```cpp
// In logic.h or a new adc_filter.h
#define LDR_FILTER_N 5
static uint16_t ldrBuf[LDR_FILTER_N];
static uint8_t ldrIdx = 0;

uint16_t readLdrMedian() {
  ldrBuf[ldrIdx++ % LDR_FILTER_N] = analogRead(PIN_LDR);
  uint16_t sorted[LDR_FILTER_N];
  memcpy(sorted, ldrBuf, sizeof(sorted));
  // insertion sort (N=5, negligible cost)
  for (int i=1;i<LDR_FILTER_N;i++){
    uint16_t v=sorted[i]; int j=i-1;
    while(j>=0 && sorted[j]>v){sorted[j+1]=sorted[j];j--;}
    sorted[j+1]=v;
  }
  return sorted[LDR_FILTER_N/2];
}
```
Call once per control loop cycle (every 100–250 ms). Replaces current `analogRead(PIN_LDR)` call.

**Flash/RAM cost:** ~200 bytes flash, 10 bytes RAM (buffer). Negligible.
**Pin cost:** None — existing GPIO34.
**CI impact:** Add one Unity test for median function (5 inputs, check median output). Maintains 20+ test coverage.

---

### R2 🔧 Software Watchdog (TWDT) Subscription (Value: HIGH / Effort: LOW)

**Why:** ESP-IDF's Task Watchdog Timer (TWDT) monitors tasks ensuring they execute within a timeout [44]. SMARTLUX's main loop could hang if the I2C bus locks up (SSD1306 or INA219 NAK storm, known ESP32 I2C issue). EcoSentinel implements I2C bus recovery and MCU soft reset with cooldowns [16]. Adding TWDT ensures SMARTLUX auto-recovers from hangs without manual reset — critical for a school demonstration running unattended.

**Implementation (firmware):**
```cpp
#include "esp_task_wdt.h"
// In setup():
esp_task_wdt_init(10, true); // 10 s timeout, panic+reset on timeout
esp_task_wdt_add(NULL);       // subscribe main task (NULL = current)

// In loop() after each major block:
esp_task_wdt_reset();
```
If the INA219 or SSD1306 hangs and loop() stops executing, the TWDT triggers a panic + system reset within 10 seconds [44]. Configure `CONFIG_ESP_TASK_WDT_PANIC=y` in sdkconfig or use the runtime API.

**Flash/RAM cost:** ~1–2 KB flash (esp_task_wdt.c), zero additional RAM.
**Pin cost:** None.
**I2C bus recovery bonus:** Add a 100 ms I2C timeout in the Wire library (`Wire.setTimeOut(100)`) before each INA219 read; on timeout, call `Wire.end(); Wire.begin(21,22);` to reinitialize — pattern from EcoSentinel [16].

---

### R3 🔧 NVS Preferences for Persistent Energy Accumulator (Value: HIGH / Effort: LOW–MEDIUM)

**Why:** SMARTLUX's E_Wh accumulator resets on every power cycle. For a practicum demonstration lasting multiple school days, losing accumulated energy data on power-off undermines the energy-savings demonstration. WLED INA219 usermod retains daily/monthly/total energy [4]. UG56-Lab-PSU uses `PreferencesManager` backed by NVS [27]. ESP32 internal flash NVS is rated for approximately 100,000 write cycles per sector [47]; with 1-minute write intervals that is ~69 days per key before wear. Writing every 60 s is therefore safe for a school practicum.

**Implementation (firmware):**
```cpp
#include <Preferences.h>
Preferences prefs;

// In setup():
prefs.begin("smartlux", false);
E_Wh      = prefs.getFloat("e_wh",      0.0f);
E_saved   = prefs.getFloat("e_saved",   0.0f);
// In energy update loop (every 60 s, not every 1 s):
static uint32_t lastNvsWrite = 0;
if (millis() - lastNvsWrite > 60000) {
  prefs.putFloat("e_wh",    E_Wh);
  prefs.putFloat("e_saved", E_saved);
  lastNvsWrite = millis();
}
// On /api/reset:
prefs.putFloat("e_wh", 0.0f);
prefs.putFloat("e_saved", 0.0f);
```
Write only on change and batch writes (open namespace once, write multiple keys, close) to minimize wear [47]. NVS uses wear levelling, distributing writes across flash pages [47]. CRC checksums protect against incomplete writes on power loss [47]. Each NVS entry is roughly 150–250 bytes of RAM overhead [37].

**Flash/RAM cost:** Preferences library is already part of arduino-esp32 SDK, ~0 additional flash; ~1 KB RAM for namespace handle. No new partition needed; default NVS partition is 16 KB (4 × 4 KB pages) [47].
**Write-endurance note:** 1-minute write interval → ~52,560 writes/year → within 100,000-cycle rating [47]. For multi-year use, increase interval to 5 minutes (8,760 writes/year).

---

### R4 🔧 PWM Dimming via LEDC on GPIO26 MOSFET (Value: MEDIUM–HIGH / Effort: LOW)

**Why:** SMARTLUX currently drives GPIO26 fully HIGH or LOW (on/off). Multiple projects implement daylight-harvesting PWM dimming: IoT-Smart-Lighting dims LED proportional to darkness [11]; BH1750-based project uses stepped brightness [12]; HA blueprint implements two-level dimming (bright → dim → off) [29]. The ESP32 LEDC peripheral supports 16 channels, 1–14 bit resolution, hardware fade [42]. GPIO26 already drives an N-MOSFET with 220–470 Ω gate resistor and 10 kΩ pull-down — this circuit is compatible with PWM at LED-appropriate frequencies.

**Implementation (firmware):**
```cpp
#include <Arduino.h>
// In setup() — replace digitalWrite setup:
ledcAttach(PIN_LAMP, 1000, 8);  // GPIO26, 1 kHz, 8-bit (0–255)
// In lamp-on state: full brightness
ledcWrite(PIN_LAMP, 255);
// In dim state (e.g., 30 s before off): dim to 40%
ledcWrite(PIN_LAMP, 102);        // 102/255 ≈ 40%
// Off:
ledcWrite(PIN_LAMP, 0);
```
LEDC at 1 kHz, 8-bit resolution is well above audible frequency and provides 256 discrete brightness levels [42]. The N-MOSFET's gate capacitance will be fully charged/discharged at 1 kHz by the 220–470 Ω gate resistor — gate charge time ≈ R×Cg ≈ 470 Ω × ~1 nF ≈ 0.5 µs, negligible vs. 1 ms period.

**Two-level dimming mode** (implementing HA blueprint pattern [29]):
- **Full ON** when dark AND motion detected → `ledcWrite(PIN_LAMP, 255)`
- **Dim** (e.g., 50%) when in motion-off timeout countdown with 15 s remaining → `ledcWrite(PIN_LAMP, 128)`
- **OFF** after timeout → `ledcWrite(PIN_LAMP, 0)`

**Energy-accuracy note:** With PWM dimming, the INA219 reads instantaneous V×I correctly because it measures actual current draw (the LED driver averages the switched current), so the existing E_Wh += V*I*dt/3600 integration remains valid.

**Flash/RAM cost:** LEDC driver is already part of ESP32 Arduino SDK — ~0 additional flash. `ledcAttach`/`ledcWrite` replace `digitalWrite` calls → net flash neutral or slightly smaller.
**Pin cost:** No new pin; GPIO26 continues to be used. Existing gate resistor and pull-down are compatible.
**Limitations:** If the LED lamp uses a constant-current driver (CC-mode driver), PWM below a certain duty may cause flicker or the driver may not respond linearly. For the 2–5 W LED lamp specified (direct low-voltage LED, not AC driver), this is not an issue at 5 V supply. Verify empirically at dim duty.

**Add to CI:** Add a logic.h unit test verifying that the dim-state transition is triggered at the correct remaining-time threshold.

---

### R5 🔧 In-Browser History Chart (Rolling Buffer) (Value: MEDIUM–HIGH / Effort: MEDIUM)

**Why:** ESP32-Terrarium implements 100-point 1-minute-resolution history charts served from a JSON endpoint `/api/history` [28]. smartclass shows per-sensor historical charts [21]. EcoSentinel has baseline-vs-saving energy comparison charts [16]. SMARTLUX's dashboard currently shows only instantaneous values via 1 s polling. A rolling energy chart would make the savings demonstration far more compelling for a school audience.

**Implementation (firmware):**
```cpp
// In smartlux.ino or a new history.h:
#define HIST_N 60  // 60 minutes
struct HistPoint { float e_wh; float e_saved; uint32_t ts; };
static HistPoint hist[HIST_N];
static uint8_t histHead = 0;

// Every 60 s in loop():
hist[histHead % HIST_N] = { E_Wh, E_saved, millis()/1000 };
histHead++;

// New endpoint GET /api/history:
// Serialize hist[] as JSON array: [{"t":0,"e":0.001,"s":0.002},...]
```
**Dashboard PROGMEM budget concern:** Current dashboard is ~8.1 KB (nearly full). The chart rendering JS must be minimal. Options:
1. Use a **pure Canvas API** approach (like mVefa/ESP32-IoT-Weather-Station [26]) — ~2–3 KB of inline JS for a polyline chart, served from a separate `/chart.js` endpoint split from the main HTML to stay within any single-string PROGMEM limit.
2. Serve chart data via `/api/history` JSON and render in existing dashboard JS with a minimal `<canvas>` polyline loop (~800 bytes of JS).
3. As an alternative to expanding PROGMEM: return history as CSV from `/api/export` for offline charting (simpler, zero PROGMEM cost).

**RAM cost:** 60 × 12 bytes ≈ 720 bytes for the ring buffer. Acceptable.
**Flash cost:** ~2–3 KB additional PROGMEM for chart JS (tight but feasible with Option 2 minimal canvas).

---

### R6 🔧 MQTT Publishing + HA Auto-Discovery (Value: MEDIUM / Effort: MEDIUM)

**Why:** WLED LDR_PIR, WLED INA219 usermod, ESPHome, Tasmota, and PIRsensorSwitch all publish to MQTT and auto-register with Home Assistant [3][4][24][30][36]. GridLessTech uses MQTT for INA219 data [6]. The JBHaMqttDiscovery library handles HA discovery payload construction, reconnect backoff, and Last Will [37]. This would allow SMARTLUX to be integrated with a school HA instance or Raspberry Pi Mosquitto broker for multi-device energy dashboards.

**Flash/RAM cost (critical at 78%):** PubSubClient adds ~15–20 KB flash. PubSubClient default buffer (256 B) is too small for discovery payloads; `setBufferSize(1024)` is required [40]. Discovery payload for SMARTLUX (~6–8 entities: motion, light_level, lamp_state, power_w, energy_wh, savings_wh, mode, override) would be ~600–800 bytes [37]. With PubSubClient the discovery buffer in JBHaMqttDiscovery grows to ~3.7 KB for 14 entities [37]. Each entity occupies ~150–250 bytes RAM [37].

**Flash feasibility:** At 78% flash (~1.14 MB used of ~1.46 MB typical 4 MB partition with OTA scheme), adding PubSubClient (~20 KB) pushes to ~79.4% — feasible but tight. Without OTA partition (single-app partition scheme, 3 MB app), more headroom is available.

**Implementation notes:**
```cpp
#include <PubSubClient.h>
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);
mqtt.setServer(MQTT_BROKER, 1883);
mqtt.setBufferSize(1024);  // required for discovery [40]

// Publish telemetry (every 5 s):
char payload[256];
snprintf(payload, sizeof(payload),
  "{\"motion\":%d,\"dark\":%d,\"lamp\":%d,\"power\":%.3f,\"energy\":%.4f,\"saved\":%.4f}",
  motionDetected, isDark, lampOn, powerW, E_Wh, E_saved);
mqtt.publish("smartlux/" DEVICE_ID "/state", payload);

// HA discovery on connect (retained):
mqtt.publish("homeassistant/sensor/smartlux_energy/config",
  "{\"name\":\"SMARTLUX Energy\",\"state_topic\":\"smartlux/.../state\","
  "\"value_template\":\"{{ value_json.energy }}\",\"unit_of_measurement\":\"Wh\",...}",
  true);  // retained=true [30][40]
```
Publish `unique_id` derived from MAC address to avoid duplicate entities [40]. Implement MQTT reconnect in loop() with non-blocking backoff (check `millis() - lastReconnect > reconnectInterval`).

**Recommendation:** Implement MQTT as a **compile-time opt-in** (`#ifdef ENABLE_MQTT`) to preserve flash budget for schools without a broker. This is consistent with WLED's usermod optional compilation pattern [3].

---

### R7 🔧 OTA Firmware Updates (Value: MEDIUM / Effort: MEDIUM — flash-constrained)

**Why:** ESP32-Terrarium, WLED, ESPHome, lumisetup all support OTA [28][43]. For a school project spanning a full year, being able to reflash SMARTLUX over WiFi without disassembly is valuable.

**Flash constraint analysis (critical):** At 78% flash on a standard `default` partition table (app0 + app1 each ~1.46 MB), OTA requires a second app partition. If currently using the `default` partition scheme (two ~1.4 MB OTA slots), 78% × 1.4 MB ≈ 1.09 MB of one slot is used. The second slot must hold the same firmware, so the same 78% = ~1.09 MB. This is feasible if the partition table has two equal OTA slots (default Arduino partition scheme `default.csv` has app0 and app1 each at 1.3 MB on a 4 MB flash).

**ArduinoOTA vs. Web OTA:**
- **ArduinoOTA** (UDP-based, same network only): ~8–12 KB flash overhead; requires Arduino IDE or `espota.py` [43]. Simplest to implement.
- **ESP32 OTA Web Update** (`/update` HTTP POST with binary upload): ~10–15 KB flash overhead; accessible from any browser on the LAN [43]; better for school use where teachers may not have Arduino IDE.

**Recommended implementation:**
```cpp
#include <ArduinoOTA.h>
// In setup():
ArduinoOTA.setHostname("smartlux");
ArduinoOTA.setPassword(OTA_PASSWORD);  // from secrets.h
ArduinoOTA.begin();
// In loop():
ArduinoOTA.handle();  // non-blocking, <1 ms normally
```
ArduinoOTA.handle() must be called in loop() — verify no blocking code prevents it. SMARTLUX already avoids `delay()` in loop, so this is compatible.

**Partition caveat:** If flash is a 4 MB chip (most ESP32 dev boards), the `default` Arduino partition scheme provides two 1.3 MB OTA slots. 1.09 MB < 1.3 MB → OTA is feasible. However, adding MQTT (+20 KB) and ArduinoOTA (+12 KB) simultaneously pushes usage to ~80–82%, approaching limits. **Recommendation: implement OTA without MQTT for the base build; add MQTT as a separate build variant.**

---

### R8 🔧 Daily/Monthly Energy Bucketing in NVS (Value: MEDIUM / Effort: LOW — builds on R3)

**Why:** WLED INA219 usermod maintains daily energy (reset after 24 h) and monthly energy (reset after 30 days) separately from total [4]. This makes the energy-savings demonstration more pedagogically clear (today's savings vs. this week's savings).

**Implementation (extends R3):**
```cpp
// Add to prefs storage:
float E_daily   = prefs.getFloat("e_daily",   0.0f);
float E_monthly = prefs.getFloat("e_monthly", 0.0f);
uint32_t dayStart   = prefs.getUInt("day_start",  0);
uint32_t monthStart = prefs.getUInt("month_start", 0);
uint32_t now = millis()/1000;
if (now - dayStart   > 86400)  { E_daily   = 0; prefs.putUInt("day_start",   now); }
if (now - monthStart > 2592000){ E_monthly = 0; prefs.putUInt("month_start", now); }
// Use NTP time (already in some comparable projects [21]) for wall-clock resets.
```
**Flash/RAM cost:** Negligible (4 more NVS keys). Builds directly on R3 infrastructure.

---

### R9 🔧 Modem Sleep / WiFi Power Optimization (Value: LOW–MEDIUM / Effort: MEDIUM)

**Why:** SMARTLUX is WiFi-connected and always-on. Espressif documentation recommends enabling WiFi modem sleep plus automatic light sleep to maintain connection while saving power [46]. In modem sleep, the CPU runs at reduced frequency with radio in DTIM sleep; measured current varies by board (modem sleep at 80 MHz: 20–31 mA datasheet spec; measured light sleep ~1.5–5 mA depending on board components) [46].

**Feasibility for SMARTLUX:** SMARTLUX must remain WiFi-connected for the web dashboard and 1 s polling. Deep sleep is incompatible with this requirement. However, **WiFi modem sleep** (automatic DTIM-based radio duty cycling while maintaining association) can be enabled with one line:
```cpp
WiFi.setSleep(true);  // enables DTIM modem sleep
```
This reduces average WiFi current without breaking connectivity. Measured savings depend on DTIM interval (typically 100 ms) and traffic pattern.

**Savings math improvement:** Modem sleep reduces ESP32 controller standby current by ~30–50 mA (from active ~80 mA to sleep ~20–30 mA) [46]. The INA219 on the 5 V rail measures total system power; the existing E_Wh accumulator already correctly accounts for this — no firmware change needed to the savings math. The savings improvement is automatic.

**Limitation:** `WiFi.setSleep(true)` can increase HTTP response latency by one DTIM interval (~100 ms), which is unnoticeable for the 1 s polling dashboard but should be noted. Light sleep (deeper) can fail when WiFi is active [46] and is not recommended here.

---

### R10 ⚙️ STRETCH: BH1750 Digital Lux Sensor (replaces LDR ADC)

**Why:** imanishraj/Smart-Street-Light uses BH1750 providing calibrated lux readings over I2C rather than a resistive voltage divider [12]. ESPHome EPO uses BH1750 at 0x23 with offset calibration [18]. BH1750 gives absolute lux values (0–65535 lx, ±20% accuracy) rather than a unitless ADC ratio, making the energy demonstration scientifically more rigorous.

**Hardware:** BH1750 breakout board (~$1–2), connects to existing I2C bus GPIO21/22 at address 0x23 (INA219 at 0x40, SSD1306 at 0x3C — no conflict). Runs on 3.3 V from ESP32. Total cost: within low-budget constraint.
**Software:** `BH1750` library (Christopher Laws), `Wire.requestFrom(0x23, 2)`.
**Constraint check:** Requires removing LDR voltage divider from GPIO34 (freeing a GPIO). I2C bus already shared — compatible. ✅ Within 5 V single-supply constraint.
**Flag:** Extra hardware → stretch goal only.

---

### R11 ⚙️ STRETCH: mmWave Radar Presence (replaces HC-SR501)

**Why:** JaiAkash-CodesIO/mmWave-radar-sensor demonstrates that the HLK-LD2410C (24 GHz mmWave) detects stationary presence via breathing detection, solving the classic PIR limitation of turning off when occupant is motionless [15]. ESPHome EPO combines PIR + mmWave for a composite occupancy sensor [18].

**Hardware cost:** HLK-LD2410C ~$5–8, UART connection to any free ESP32 UART pins. Requires extra hardware → stretch goal only.
**Flag:** Extra hardware → stretch goal only. The HC-SR501 is entirely adequate for the school practicum scope.

---

## Part VI — Implementation Priority Matrix

| Rank | Recommendation | Hardware | Effort | Flash Cost | Value | Priority |
|---|---|---|---|---|---|---|
| R1 | LDR ADC median filter | None | 1 h | ~200 B | Robustness | **NOW** |
| R2 | TWDT watchdog subscription | None | 1 h | ~1–2 KB | Robustness | **NOW** |
| R3 | NVS energy persistence | None | 2 h | 0 B (SDK) | Energy accuracy | **NOW** |
| R4 | LEDC PWM dimming (2-level) | None | 2–3 h | ~0 B | Energy + UX | **SHORT TERM** |
| R5 | In-browser history chart | None | 4–6 h | ~2–3 KB PROGMEM | Dashboard UX | **SHORT TERM** |
| R6 | MQTT + HA discovery (opt-in) | None | 4–6 h | ~20 KB | Integration | **MEDIUM TERM** |
| R7 | OTA updates (ArduinoOTA) | None | 2 h | ~12 KB | Maintenance | **MEDIUM TERM** |
| R8 | Daily/monthly energy buckets | None | 1 h | ~0 B | Energy UX | **WITH R3** |
| R9 | WiFi modem sleep | None | 0.5 h | 0 B | Power | **OPTIONAL** |
| R10 | BH1750 digital lux | +$2 hw | 2 h | ~3 KB | Science rigor | **STRETCH** |
| R11 | mmWave radar | +$7 hw | 4 h | ~5 KB | Presence quality | **STRETCH** |

---

## Part VII — What SMARTLUX Should NOT Change

Based on the ecosystem survey, the following SMARTLUX practices are superior to all comparable projects and should be preserved:

1. **INA219 real power metering vs. rated-wattage estimation:** The JEESR paper used rated 9W × ON-intervals without an energy meter and explicitly noted this is an upper-bound estimate [48]. SMARTLUX's actual V×I×dt/3600 integration is methodologically correct.

2. **Explicit 11-state FSM with observability keys:** No comparable project uses a formal state machine; most use ad-hoc if/else. SMARTLUX's approach makes testing and debugging significantly easier.

3. **20/20 Unity native unit tests + GitHub Actions CI:** Zero other look-alike projects have this. The CI guard on dashboard size (`ino2cpp` guard) is especially novel.

4. **CSP `default-src 'none'` + session-cookie token auth + rate limiting:** No comparable project has any of these. The smart_socket project uses hardcoded `admin/admin` credentials [25]; smartclass hard-codes Firebase API keys in public source [21]. SMARTLUX's security posture is demonstrably better.

5. **Hysteresis darkness threshold (10–20%):** WLED LDR_PIR also uses dual thresholds [3][36], but SMARTLUX's empirical calibration with documented methodology is better practice than fixed 0.4/0.5 defaults.

6. **60s PIR warm-up with H-mode retrigger:** Tasmota documentation confirms H-mode (retrigger) is correct for HC-SR501 with SwitchMode 1 [30]; SMARTLUX already implements this correctly.

7. **OpenSCAD 3D-printable parametric enclosure + SECURITY.md + changelog + calibration docs:** No look-alike project has any of these. These should be highlighted as SMARTLUX's strongest differentiators for the practicum submission.

---

## Citations

[1] hari2k7/smart-lighting-system — https://github.com/hari2k7/smart-lighting-system
[2] KeerthiYarashi/Smart-Street-Lighting-System — https://github.com/KeerthiYarashi/Smart-Street-Lighting-System
[3] WLED LDR_PIR README — https://github.com/wled/WLED/blob/4b5ceca.../usermods/LDR_PIR/README.md
[4] WLED INA219 Readme — https://github.com/wled/WLED/blob/ee9d04c.../usermods/INA219/Readme.md
[5] Botunade/MotionLights — https://github.com/Botunade/MotionLights
[6] GridLessTech/solar-bottle-lights-monitor — https://github.com/GridLessTech/solar-bottle-lights-monitor
[7] Rggitty/edge-ai-security-camera-node — https://github.com/Rggitty/edge-ai-security-camera-node
[8] bjoernellens1/ESP32-PowerGuard — https://github.com/bjoernellens1/ESP32-PowerGuard
[9] GitHub topic: smart-home-lighting — https://github.com/topics/smart-home-lighting
[10] GitHub topic: smart-lighting — https://github.com/topics/smart-lighting
[11] Arlene-Riona/IoT-Smart-Lighting — https://github.com/Arlene-Riona/IoT-Smart-Lighting
[12] imanishraj/Smart-Street-Light-System — https://github.com/imanishraj/Smart-Street-Light-System
[13] jackshiels/LightSense — https://github.com/jackshiels/LightSense
[14] parikshitpagare/smart-home-automation-rtos — https://github.com/parikshitpagare/smart-home-automation-rtos
[15] JaiAkash-CodesIO/mmWave-radar-sensor — https://github.com/JaiAkash-CodesIO/mmWave-radar-sensor
[16] Atlas-did/EcoSentinel — https://github.com/Atlas-did/EcoSentinel
[17] WLED PR #4823 LDR_PIR — https://github.com/wled/WLED/pull/4823/files
[18] EverythingSmartHome/everything-presence-one YAML — https://github.com/EverythingSmartHome/everything-presence-one/blob/234a5db.../everything-presence-one.yaml
[19] Tasmota Discussion #17519 PIR+light — https://github.com/arendst/Tasmota/discussions/17519
[20] Bekaidarchik/smart-energy-monitoring-system — https://github.com/Bekaidarchik/smart-energy-monitoring-system
[21] prakashkarkee/smartclass — https://github.com/prakashkarkee/smartclass
[22] GR4Yxx/AutomaticCollegeLights-Arduino — https://github.com/GR4Yxx/AutomaticCollegeLights-Arduino
[23] pedrogbmendes/SCDTR-Project — https://github.com/pedrogbmendes/SCDTR-Project
[24] WLED MoonModules PIRsensorSwitch — https://mm.kno.wled.ge/usermods/PIRsensorSwitch
[25] jkskw/smart_socket — https://github.com/jkskw/smart_socket
[26] mVefa/ESP32-IoT-Weather-Station — https://github.com/mVefa/ESP32-IoT-Weather-Station
[27] universalgeek56/UG56-Lab-PSU — https://github.com/universalgeek56/UG56-Lab-PSU
[28] Rau7han/ESP32-Terrarium — https://github.com/Rau7han/ESP32-Terrarium
[29] edwardtfn/HomeAssistant-Config blueprint — https://github.com/edwardtfn/HomeAssistant-Config/blob/main/blueprints/automation/EdwardTFN/light_motion_illuminance_2_levels.yaml
[30] Tasmota PIR Motion Sensors — https://tasmota.github.io/docs/PIR-Motion-Sensors
[31] ESPHome Binary Sensor — https://esphome.io/components/binary_sensor/
[32] Tasmota Power Monitoring Calibration — https://tasmota.github.io/docs/Power-Monitoring-Calibration
[33] SumaitaB/IlluminateTech — https://github.com/SumaitaB/IlluminateTech-Smart-Classroom-Lighting-Solutions
[34] pranjal-joshi/Radiance — https://github.com/pranjal-joshi/Radiance
[35] egalistel-dev/lumisetup-esp8266 — https://github.com/egalistel-dev/lumisetup-esp8266
[36] WLED LDR_PIR.cpp — https://github.com/wled/WLED/blob/4b5ceca.../usermods/LDR_PIR/LDR_PIR.cpp
[37] jonnybergdahl/Arduino_JBHaMqttDiscovery_Library — https://github.com/jonnybergdahl/Arduino_JBHaMqttDiscovery_Library
[38] scottbez1/ESP32_INA219_PowerMonitor CI — https://github.com/scottbez1/ESP32_INA219_PowerMonitor/blob/master/.travis.yml
[39] ayush-gangwar-09/Smart-Street-Light-IoT — https://github.com/ayush-gangwar-09/Smart-Street-Light-IoT
[40] ESP32 HA via MQTT Discovery — https://esp32hub.com/en/projects/home-assistant-mqtt-discovery
[41] ESPHome Median Filter — https://esphome.io/components/sensor/filter/median
[42] ESP32 LEDC Arduino docs — https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html
[43] ESP32 OTA Web Update docs — https://docs.espressif.com/projects/arduino-esp32/en/latest/ota_web_update.html
[44] ESP-IDF Watchdogs — https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html
[45] ESP-IDF NVS Flash — https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
[46] Mischianti ESP32 Power Saving — https://mischianti.org/esp32-power-saving-modem-and-light-sleep-2
[47] Zbotic ESP32 NVS Flash — https://zbotic.in/esp32-nvs-flash-storage-persist-settings-between-reboots/
[48] JEESR Vol.29 Article 16 — https://jeesr.uitm.edu.my/v1/JEESR/Vol.29/article16.pdf

---

*Report prepared for SMAN CMBBS Kelompok 2 — SMARTLUX 2026/2027. All recommendations respect hard constraints: single regulated 5 V supply, ESP32 ADC max 3.3 V, low budget, school-practicum-appropriate simplicity.*

## Full citation URLs (untruncated)

- https://github.com/hari2k7/smart-lighting-system
- https://github.com/KeerthiYarashi/Smart-Street-Lighting-System
- https://github.com/wled/WLED/blob/4b5ceca559db4e3d38c72c1b88df6f057d50458f/usermods/LDR_PIR/README.md
- https://github.com/wled/WLED/blob/ee9d04c1b2352be229d61a4b853ebb33014b0650/usermods/INA219/Readme.md
- https://github.com/Botunade/MotionLights
- https://github.com/GridLessTech/solar-bottle-lights-monitor
- https://github.com/Rggitty/edge-ai-security-camera-node
- https://github.com/bjoernellens1/ESP32-PowerGuard
- https://github.com/topics/smart-home-lighting
- https://github.com/topics/smart-lighting
- https://github.com/Arlene-Riona/IoT-Smart-Lighting
- https://github.com/imanishraj/Smart-Street-Light-System
- https://github.com/jackshiels/LightSense
- https://github.com/parikshitpagare/smart-home-automation-rtos
- https://github.com/JaiAkash-CodesIO/mmWave-radar-sensor
- https://github.com/Atlas-did/EcoSentinel
- https://github.com/wled/WLED/pull/4823/files
- https://github.com/EverythingSmartHome/everything-presence-one/blob/234a5db43c9a894535fb35e7e31e7c7332b52098/everything-presence-one.yaml
- https://github.com/arendst/Tasmota/discussions/17519
- https://github.com/Bekaidarchik/smart-energy-monitoring-system
- https://github.com/prakashkarkee/smartclass/blob/main
- https://github.com/GR4Yxx/AutomaticCollegeLights-Arduino
- https://github.com/pedrogbmendes/SCDTR-Project
- https://mm.kno.wled.ge/usermods/PIRsensorSwitch
- https://github.com/jkskw/smart_socket
- https://github.com/mVefa/ESP32-IoT-Weather-Station
- https://github.com/universalgeek56/UG56-Lab-PSU
- https://github.com/Rau7han/ESP32-Terrarium
- https://github.com/edwardtfn/HomeAssistant-Config/blob/main/blueprints/automation/EdwardTFN/light_motion_illuminance_2_levels.yaml
- https://tasmota.github.io/docs/PIR-Motion-Sensors
- https://esphome.io/components/binary_sensor/
- https://tasmota.github.io/docs/Power-Monitoring-Calibration
- https://github.com/SumaitaB/IlluminateTech-Smart-Classroom-Lighting-Solutions
- https://github.com/pranjal-joshi/Radiance
- https://github.com/egalistel-dev/lumisetup-esp8266
- https://github.com/wled/WLED/blob/4b5ceca559db4e3d38c72c1b88df6f057d50458f/usermods/LDR_PIR/LDR_PIR.cpp
- https://github.com/jonnybergdahl/Arduino_JBHaMqttDiscovery_Library
- https://github.com/scottbez1/ESP32_INA219_PowerMonitor/blob/master/.travis.yml
- https://github.com/ayush-gangwar-09/Smart-Street-Light-IoT
- https://esp32hub.com/en/projects/home-assistant-mqtt-discovery
- https://esphome.io/components/sensor/filter/median
- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/ledc.html
- https://docs.espressif.com/projects/arduino-esp32/en/latest/ota_web_update.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/wdts.html
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/nvs_flash.html
- https://mischianti.org/esp32-power-saving-modem-and-light-sleep-2
- https://zbotic.in/esp32-nvs-flash-storage-persist-settings-between-reboots/
- https://jeesr.uitm.edu.my/v1/JEESR/Vol.29/article16.pdf
