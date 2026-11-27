# Implementation Plan: R6 — Opt-in MQTT Publishing + Home Assistant Discovery (SMARTLUX)

Generated: 2026-11-24. Planning-only document — no production code has been changed.

## Provenance / constraints discovered

- `/plan`'s required `ccg-workflow` runtime is **absent** (no `~/.claude/bin/codeagent-wrapper`, no `~/.claude/.ccg/prompts`, no Codex/Antigravity CLIs). Per `AGENTS.md`, the six phases were run locally: full repo context retrieval, then multi-model analysis via Council seats `councillor-beam` (feasibility), `councillor-deepseek` (HA design), `councillor-mimo` (testing/CI). `councillor-glm` failed twice on an unavailable preset and was marked failed. No external model had filesystem write access.
- R6 is the **only deliberately deferred** item in `docs/SMARTLUX-OpenSource-Comparison-Report.md` (priority matrix line 538: no hardware, 4–6 h, ~20 KB flash, MEDIUM TERM). R1–R5 + OTA are already implemented.
- Hard constraints from `AGENTS.md`: single regulated 5 V supply, ESP32 ADC ≤ 3.3 V, no new hardware; keep pins GPIO34/27/26/21/22/32, the 4 original HTTP routes + `/api/history`, `/api/export`, `/chart`, the status JSON key set, the 8 KB dashboard / 2.5 KB chart PROGMEM budgets, and no `delay()` in `loop()`.
- **The status JSON has 24 keys in source** (`SMARTLUX.ino:1093–1110`, `lamp_pwm` added in R4); docs/README saying "23" are stale. Never hardcode a count — derive keys from the formatter.
- `secrets.h` is git-ignored (`.gitignore:5`) and included via `#if __has_include("secrets.h")`. Established opt-in pattern: `SMARTLUX_OTA_PASS` (OTA only when ≥ 8 chars) and `SMARTLUX_TOKEN`.
- Verified baseline builds: `arduino-cli` 1,050,891 B flash (80 %) / 53,084 B RAM (16 %); `pio run` 1,032,019 B (78.7 %) / 52,156 B (15.9 %). CI guard: `.github/workflows/ci.yml` lines 23–47 (PROGMEM literal sizes + ino2cpp column-0 `function` trap).
- Verified from installed ESP32 core 3.3.12 source: `NetworkClient::setConnectionTimeout(uint32_t)` sets the connect timeout `_timeout`; `Stream::setTimeout()` only sets the stream read wait. Use `setConnectionTimeout()` to bound the TCP connect, never `setTimeout()`.

## Analysis perspectives (reconciled)

- **Feasibility (beam):** PubSubClient 2.8 is the only library that keeps a combined MQTT+OTA build feasible (~15–20 KB vs ~150–250 KB for AsyncMqttClient). Compile-time opt-in keeps the base build byte-identical. `connect()` is the one blocking surface and must be bounded; TLS fits flash but is operationally out for v1.
- **Testing/CI (mimo):** RED-first pure helpers; a new `tests/repo/mqtt.test.js`; close the pre-existing Node gap for `CHART_HTML`; keep the entity table in a `static inline` accessor so it cannot leak into the base build's rodata; pin the real key set from source; 90 % flash ceiling across four build variants.
- **HA design (deepseek):** publish the existing status JSON verbatim through one shared `buildStatusJson()`; use 6-hex MAC IDs, nested discovery topics, 1536 B buffer, raw-HA-level semantics (`pir` not `g_motion`, read-only `binary_sensor` for the lamp), `expire_after`, LWT QoS-1 will, and local-broker/ACL privacy controls.

## Task type

- [x] Backend / firmware (ESP32), with a small docs + CI + native-test surface.
- [ ] Frontend (dashboard is byte-stable — do not touch `INDEX_HTML` or `CHART_HTML`).

## Technical solution (recommended)

Implement MQTT as a **compile-time opt-in** exactly like OTA, so the base build for schools without a broker stays byte-for-byte unchanged (feature compiled out → zero flash/RAM cost). Strategy **A: mirror `/api/status`** — one formatter, no duplicated derivation, unit conversion done in HA templates.

1. **Gating & configuration.** In git-ignored `secrets.h`: `SMARTLUX_ENABLE_MQTT`, `MQTT_BROKER`, optional `MQTT_PORT` (1883), `MQTT_USER`/`MQTT_PASS`, `MQTT_BASE_TOPIC` (`smartlux`), `MQTT_DISCOVERY_PREFIX` (`homeassistant`). All MQTT code lives in `firmware/SMARTLUX/mqtt.h` wrapped in `#if defined(SMARTLUX_ENABLE_MQTT)`. Missing broker or placeholder `GANTI_BROKER` ⇒ `#warning` + disabled; `MQTT_USER` without `MQTT_PASS` ⇒ `#error`. Document in `secrets.example.h` as commented placeholders; ESP32 `WiFiClient` does **not** resolve `.local` mDNS — use an IP or router DNS name.
2. **Library & pinning.** `knolleary/PubSubClient@2.8` with **`setBufferSize(1536)`** (default 256 B truncates discovery; 1024 B is too tight once `device`, `origin`, attributes and `expire_after` are included). Pin exactly (no caret) in `platformio.ini` `lib_deps` and `sketch.yaml` `libraries`. Use `R"json(` — never `R"rawliteral(` — for MQTT string literals, because `tests/repo/firmware.test.js` grabs the first raw-runtime literal for the dashboard budget.
3. **One shared status formatter.** Extract the existing `handleStatus()` `snprintf` into `buildStatusJson(char*, size_t)` in `SMARTLUX.ino`, returning byte count or `-1` on truncation; HTTP and MQTT both call it, so `/api/status` bytes stay identical and no second conversion table exists. Pure, host-testable helpers in `logic.h` (no Arduino deps): 6-hex `mqttDeviceId()`, topic builders, bounded `buildMqttDiscoveryConfig()` (each config `< ~900 B`), entity table in a `static inline` accessor with a function-local `static const` array (a file-scope table in a header would leak rodata into the base build), rollover-safe `mqttShouldAttempt()`/`mqttShouldPublish()`, `mqttNextBackoffMs()` (5 s → 30 s cap), and `mqttConfigured()`. Native suite grows 28 → ~37.
4. **Board-only glue with no global coupling.** `mqtt.h` is included at the top of `.ino`, **before `g_net` is declared (line 166)**, so it must not reference `g_net`/`g_ip` directly. Expose `mqttPoll(bool networkReady, const char *stateJson, size_t len)`; the `.ino` wrapper computes `networkReady = (g_net == NET_STA && WiFi.status() == WL_CONNECTED)`, calls `buildStatusJson()` only when an MQTT publish is due, and calls `mqttPoll()` **after `server.handleClient()`** so a 2 s connect cannot delay an in-flight dashboard request. Bound the connect with `wifiClient.setConnectionTimeout(2000)`; prefer an IP literal or one bounded `WiFi.hostByName()`. Call `mqtt.will()` **before** `connect()`. No `delay()` in `loop()`.
5. **Discovery & LWT.** `<id>` = 6 lowercase hex from the low 24 bits of `ESP.getEfuseMac()` (4-hex gives ~0.7 % collision across 30 classroom devices). Client ID `smartlux-<id>`. Connect with a retained QoS-1 will `offline` to `smartlux/<id>/availability`. On **each successful (re)connect only** publish, in order: retained per-entity configs → retained `online` → one immediate state. Use nested topics `homeassistant/<component>/smartlux_<id>/<object>/config` (ACL-friendly, avoids flat collisions), `~` abbreviation for the base topic, and a shared `device`/`origin` block. Never publish discovery on the 5 s tick. `expire_after: 60` covers the retained-`online` edge case when the device reboots into AP mode. Every `publish()` return value is checked and logged on failure.
6. **Telemetry.** Every 5 s publish the shared status JSON verbatim (non-retained, QoS 0) to `smartlux/<id>/state`; HA `value_template`s do `power_mw/1000`, `lamp_pwm/255*100`, etc. No new CSV columns, no JSON-key changes, no dashboard bytes changed.
7. **Entities (8 core + 1 optional diagnostic).** `motion` (`binary_sensor`, `device_class: motion`) uses raw `pir`, **not** `g_motion` (30 s window, not in JSON); illuminance (`lux_est`, unit `lx`, label "Iluminasi (estimasi)"); lamp (`binary_sensor`, `device_class: light`, read-only — no HA `light` platform without a command topic); power (`power_mw` → W); energy (`energy_wh`, `state_class: total_increasing`, label "Energi Sistem"); savings (`savings_pct`, label "Penghematan (vs selalu nyala)"); mode (`device_class: enum`, `options: [auto,on,off]`, no `json_attributes_template` so all FSM keys appear as attributes — HA ≥ 2023.11); override derived in-template from `mode != 'auto'`; optional PWM diagnostic. Do **not** add `savings_wh` in v1 (not in `/api/status`; if later required, add `savingsWh()` to `logic.h` and expose in both HTTP and MQTT through the shared builder).
8. **In scope vs out.** IN: read-only telemetry + discovery + LWT + opt-in gating + docs/tests/CI. OUT (document as future ideas): MQTT command/control of lamp or mode (dashboard remains the control surface), TLS (no NTP/SNTP in firmware, CA rotation, ~40–60 KB flash and ~35–45 KB handshake RAM — `setInsecure()` is encryption without authentication, not a security control), cloud brokers, HA device-based discovery (HA ≥ 2024.11 and ~2.5–3.5 KB payload, over buffer), daily/monthly buckets, changing sensor/PWM/NVS logic. In AP fallback MQTT stays idle with one Serial note.
9. **Security & privacy.** Broker on trusted/IoT VLAN only; dedicated low-privilege broker account with per-device ACL (`readwrite smartlux/<id>/#`, `write homeassistant/+/smartlux_<id>/+/#`); never reuse `SMARTLUX_TOKEN`/`SMARTLUX_OTA_PASS` as MQTT password; `secrets.h` is compiled into `.bin` (`strings` recovers it) — treat the password as sensitive and rotate on retirement; local broker only, never cloud; short HA recorder retention for occupancy data and no student names in `device.name`; publish only the 6-hex suffix, never full MAC or dashboard token. Plain 1883 is acceptable only on the trusted/IoT LAN.

## Implementation steps

| # | Step | Deliverable |
|---|------|-------------|
| 1 | Add the opt-in MQTT block (commented) to `firmware/SMARTLUX/secrets.example.h`; verify `secrets.h` stays git-ignored. | Secrets template documents MQTT without committing credentials. |
| 2 | Pin `PubSubClient@2.8` in `platformio.ini` and `sketch.yaml`; add `esp32dev-mqtt` env with dummy broker creds via `build_flags` only. | Both build systems resolve the same library; CI needs no `secrets.h`. |
| 3 | RED: append pure MQTT helpers + entity table to `logic.h` behind new Unity tests; watch them fail to compile (established repo RED). | Failing tests define the expected API. |
| 4 | GREEN: implement helpers in `logic.h`; get `pio test -d tests/native -e native` green (~37/37). | Pure rules covered on the host. |
| 5 | Extract `buildStatusJson()` from `handleStatus()` in `SMARTLUX.ino`; HTTP output byte-identical. | One formatter for HTTP + MQTT; no `handleStatus()` behaviour change. |
| 6 | Add `firmware/SMARTLUX/mqtt.h` with guarded glue (`mqttPoll(networkReady, json, len)`, retained discovery, LWT, backoff). | New header, compiled only when opted in, no `g_*` references. |
| 7 | Wire into `SMARTLUX.ino`: include `mqtt.h`; one wrapper in `loop()` after `server.handleClient()` that computes readiness, builds JSON only when due, then calls `mqttPoll()`. No other `.ino` changes. | Minimal diff; base build still compiles MQTT out. |
| 8 | Add `tests/repo/mqtt.test.js` (~10 invariants) and extend `tests/README.md`. | 18 → ~28 repo tests covering gating, secrets hygiene, pins, buffer, no-delay, raw-literal trap, 24-key pin + MQTT subset. |
| 9 | CI: compile four variants (base, MQTT-only, OTA-only, MQTT+OTA combined), fail > ~90 % of the ~1,310,720 B app slot (≈1,179,648 B) and fail if base flash grows > ~100 B; add raw-literal count check. | All opt-in combinations proven; zero-cost base enforced. |
| 10 | Docs sync in Indonesian: `firmware/SMARTLUX/README.md`, `CHANGELOG.md`, `.github/SECURITY.md`, comparison-report R6 status, and the proposal's BAB II/III + Lampiran 5 (including the occupancy-privacy note in K3). | Proposal and reports reflect R6 as opt-in; 23→24 key wording fixed. |
| 11 | Verification (below), then stop. Physical LAN broker + HA test stays an open hardware gate. | Plan complete; no hardware claim made. |

## Pseudo-code (core shape)

```cpp
// logic.h — pure, host-testable; every helper static inline
static inline bool mqttDeviceId(uint64_t mac, char *out, size_t n);   // 6 lowercase hex
static inline bool mqttTopic(int kind, const char *id, const char *component,
                             const char *object, char *out, size_t n);
static inline bool mqttShouldAttempt(unsigned long now, unsigned long last, unsigned long delayMs);
static inline unsigned long mqttNextBackoffMs(unsigned int attempt);  // 5,10,20,30s cap
static inline int buildMqttDiscoveryConfig(const MqttEntity *e, const char *id,
                                           char *out, size_t n);      // -1 if truncated
// accessor keeps the table out of the base build's rodata when MQTT is off
static inline const MqttEntity *mqttEntities(size_t *count) {
  static const MqttEntity kEntities[] = { /* 8 core + 1 optional */ };
  *count = sizeof(kEntities) / sizeof(kEntities[0]); return kEntities;
}

// SMARTLUX.ino — one formatter, shared by HTTP and MQTT
static int buildStatusJson(char *json, size_t n) {
  const int w = snprintf(json, n, /* existing 24-key format string */, /* ... */);
  return (w > 0 && (size_t)w < n) ? w : -1;
}
static void handleStatus() { /* as today, but calls buildStatusJson(); 500 on -1 */ }

#if defined(SMARTLUX_ENABLE_MQTT)
static void pollMqtt() {
  const bool ready = (g_net == NET_STA) && (WiFi.status() == WL_CONNECTED);
  if (mqttWantsState(ready)) {
    char json[768];
    const int n = buildStatusJson(json, sizeof(json));
    mqttPoll(ready, json, n > 0 ? (size_t)n : 0);
  } else mqttPoll(ready, nullptr, 0);   // still runs keepalive/reconnect
}
#endif
// call pollMqtt() at the end of loop(), after server.handleClient()

// mqtt.h — board-only, compiled only under #if defined(SMARTLUX_ENABLE_MQTT)
WiFiClient mqttNet;
PubSubClient mqtt(mqttNet);
void mqttBegin() {
  mqtt.setClient(mqttNet); mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setBufferSize(1536);
  mqttNet.setConnectionTimeout(2000); // TCP connect bound — NOT Stream::setTimeout()
}
void mqttPoll(bool ready, const char *stateJson, size_t len) {
  if (!ready) { /* idle in AP; no reconnect churn */ return; }
  if (!mqtt.connected()) {
    if (!mqttShouldAttempt(millis(), g_lastTry, g_delay)) return;
    g_lastTry = millis();
    if (mqtt.connect(g_clientId, MQTT_USER, MQTT_PASS,
                     availTopic, 1, true, "offline")) {  // LWT set before connect
      g_attempts = 0;
      publishDiscoveryOnce();        // retained configs, then retained "online"
      publishNow(stateJson, len);    // immediate state
    } else { ++g_attempts; g_delay = mqttNextBackoffMs(g_attempts); return; }
  }
  mqtt.loop();
  if (stateJson && len && millis() - g_lastPub >= 5000UL) {
    g_lastPub = millis();
    if (!mqtt.publish(stateTopic, (uint8_t*)stateJson, len, false))
      Serial.println(F("# MQTT publish gagal"));
  }
}
```

## Entity map (HA)

| Component | Object | Source key / template | Notable config |
|---|---|---|---|
| `binary_sensor` | `motion` | `value_json.pir == 1` | `device_class: motion`, attribute `pir_state` |
| `sensor` | `illuminance` | `value_json.lux_est` | `device_class: illuminance`, `lx`, attributes `light_state/light_pct/light_adc`, label "Iluminasi (estimasi)" |
| `binary_sensor` | `lamp` | `value_json.lamp == 1` | `device_class: light`, read-only, attributes `lamp_state/lamp_pwm` |
| `sensor` | `power` | `value_json.power_mw / 1000` | `device_class: power`, `W`, attributes `voltage_v/current_ma` |
| `sensor` | `energy` | `value_json.energy_wh` | `device_class: energy`, `Wh`, `total_increasing` |
| `sensor` | `savings` | `value_json.savings_pct` | `%`, `mdi:leaf`, label "Penghematan (vs selalu nyala)" |
| `sensor` | `mode` | `value_json.mode` | `device_class: enum`, `options: [auto,on,off]`; no attributes template ⇒ all FSM keys visible |
| `binary_sensor` | `override` | `value_json.mode != 'auto'` | `mdi:hand-back-right` |
| `sensor` (opt.) | `pwm` | `value_json.lamp_pwm / 255 * 100` | `%`, `entity_category: diagnostic` |

All configs: nested topic `homeassistant/<component>/smartlux_<id>/<object>/config`, retained; shared `device` block (`identifiers: ["smartlux_<id>"]`, `configuration_url: http://smartlux.local/`) and `origin`; `~` base-topic abbreviation; `state_topic: ~/state`; `availability_topic: ~/availability`; `expire_after: 60`. v1 has **no** `command_topic`.

## Key files

| File | Operation | Description |
|------|-----------|-------------|
| `firmware/SMARTLUX/logic.h` | Modify | Pure MQTT helpers + entity table accessor (host-testable). |
| `firmware/SMARTLUX/mqtt.h` | Create | Guarded board glue: PubSubClient, reconnect, discovery, LWT, backoff; no `g_*` coupling. |
| `firmware/SMARTLUX/SMARTLUX.ino` | Modify (minimal) | Include `mqtt.h`; extract `buildStatusJson()`; one `pollMqtt()` wrapper after `server.handleClient()`. |
| `firmware/SMARTLUX/secrets.example.h` | Modify | Document all MQTT defines as commented placeholders. |
| `firmware/SMARTLUX/platformio.ini` | Modify | Pin `PubSubClient@2.8`; add `esp32dev-mqtt` env. |
| `firmware/SMARTLUX/sketch.yaml` | Modify | Pin `PubSubClient (2.8)`. |
| `tests/native/test/test_logic/test_logic.cpp` | Modify | RED-first tests for pure helpers (~37 total). |
| `tests/repo/mqtt.test.js` | Create | ~10 invariants: gating, commented-only secrets, no hardcoded credentials, exact pins, `setBufferSize(1536)`, no-delay, `R"json(`, 24-key pin + MQTT subset, raw-literal count, base-build symbol isolation. |
| `tests/README.md` | Modify | Add `mqtt.test.js` to the test inventory. |
| `.github/workflows/ci.yml` | Modify | Four build variants under a 90 % flash ceiling; keep base guard intact; add raw-literal count check. |
| `firmware/SMARTLUX/README.md`, `CHANGELOG.md`, `.github/SECURITY.md`, `docs/*`, `proposal_SMARTLUX_Kelompok_2.md` | Modify (Indonesian) | Docs sync; proposal edits line-targeted — **never reformat the two base64 image lines (currently ~356 / ~360; re-locate before editing)**. |

## Risks and mitigation

| Risk | Mitigation |
|------|------------|
| Flash headroom at 80 % base; ~20–24 KB for MQTT, OTA another ~12 KB | Base compiles MQTT out (zero cost). CI compiles base/MQTT/OTA/combined; fail any > ~90 % of slot. If combined is too tight, document choosing one opt-in per build (do not silently drop either). |
| PubSubClient 256 B buffer truncates discovery | `setBufferSize(1536)`; keep each config `< ~900 B`; check every `publish()` return; repo test asserts the buffer call. |
| Blocking TCP connect starves the 30 s TWDT | `setConnectionTimeout(2000)` (verified from ESP32 core 3.3.12 source; `setTimeout()` is read-only wait). Prefer IP literal or bounded `WiFi.hostByName()`. `mqttPoll()` after `server.handleClient()`, no `delay()`; repo test greps both files. |
| `mqtt.h` included before `g_net` declaration | Pass `bool networkReady` and JSON buffer; `.ino` wrapper owns globals. |
| Broker absent in AP fallback → reconnect churn | Gate on `NET_STA`; idle with one Serial note in AP; backoff caps at 30 s. |
| Credential leakage (`secrets.h` compiled into `.bin`) | Git-ignored `secrets.h`; commented `secrets.example.h`; dedicated low-privilege broker account + ACL; never reuse dashboard/OTA secrets; repo test scans tracked files. |
| Retained discovery storms / duplicate HA entities | Configs once per (re)connect, retained, stable 6-hex `unique_id`, nested topics; `expire_after: 60`; never on the 5 s tick. |
| Drift between `/api/status` and MQTT field names | One `buildStatusJson()`; repo test parses the real 24-key format string and asserts every `value_json.X` in the entity table exists there; MQTT keys are a subset. |
| HA version differences | Document `device_class: enum` (HA ≥ 2023.11) fallback; device-based discovery deliberately not used. |
| Occupancy privacy / cloud leakage | Local broker only, ACL, short recorder retention, no student names, no full MAC, no command topics. |
| Proposal base64 lines destroyed by a formatter | Line-targeted edits only; verify both `data:image/png;base64` lines still present after any proposal edit. |

## Verification sequence (exact commands)

```
# RED first: append ~9 Unity tests, expect a compile failure naming the first missing helper
pio test -d tests/native -e native

# CI guard (same Python block as ci.yml lines 26-47) + raw-literal count
python3 - <<'PY'
import re
src = open("firmware/SMARTLUX/SMARTLUX.ino", encoding="utf-8").read()
budgets = {"INDEX_HTML": 8192, "CHART_HTML": 2560}
assert src.count('R"rawliteral(') == 2, "unexpected new raw literal"
for name, budget in budgets.items():
    m = re.search(re.escape(name) + r'\[\][^=]*=\s*R"rawliteral\((.*?)\)rawliteral"', src, re.S)
    assert m, name
    assert len(m.group(1)) <= budget, name
assert not [l for l in src.splitlines() if re.match(r'^(async\s+)?function\s', l)]
print("guard OK")
PY

# Base build — must match baseline 1,050,891 B flash / 53,084 B RAM (arduino-cli)
#              and 1,032,019 B / 52,156 B (pio); no >~100 B growth
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX
arduino-cli compile --profile smartlux firmware/SMARTLUX
pio run -d firmware/SMARTLUX

# Opt-in builds (no secrets.h, no real credential in CI)
pio run -d firmware/SMARTLUX -e esp32dev-mqtt
# plus OTA-only and MQTT+OTA combined variants, each < 90 % of the app slot

# Host tests (28 → ~37 native; 18 → ~28 repo, auto-globbed)
pio test -d tests/native -e native
node --test tests/repo/*.test.js

# Static spot-checks
grep -c 'R"rawliteral(' firmware/SMARTLUX/SMARTLUX.ino                     # = 2
grep -rnE '^\s*#define\s+MQTT_(PASS|USER|BROKER)\s*"' firmware/SMARTLUX/secrets.example.h  # = 0 uncommented
git ls-files | grep -x 'firmware/SMARTLUX/secrets.h'                      # no output

# Proposal integrity (after doc step): exactly two base64 image lines still present
grep -c 'data:image/png;base64' proposal_SMARTLUX_Kelompok_2.md           # = 2
```

Hardware/LAN gate (not closable here): run a broker on the trusted VLAN with persistence + ACL; flash the opt-in build; with `mosquitto_sub -t 'homeassistant/#' -v` and `-t 'smartlux/<id>/#' -v` confirm retained discovery entities appear once per connect, telemetry updates every 5 s, LWT flips to `offline` on power loss and `online` after reboot, HA entities render, and the 1 Hz CSV/dashboard remain unaffected. Also verify the combined MQTT+OTA build stays under 90 % flash.

## Stop condition

Stop after step 11 once the base and all opt-in builds compile under the flash ceiling, all native/repo tests pass, the CI guard passes, the docs are synced in Indonesian, and the two proposal base64 lines are intact. Do not attempt physical LAN/HA verification in this environment. Execution is a separate request (`/ccg:execute` is unavailable here — run the steps directly or via local subagents).

## Session handoff

- CODEX_SESSION: N/A — Codex/Antigravity runtime absent.
- ANTIGRAVITY_SESSION: N/A — Codex/Antigravity runtime absent.
- Council analysis seats (read-only): `councillor-beam` = `ses_df21f7de9ffeu0P9w2XNF3SzrO`, `councillor-deepseek` = `ses_df21f7ddaffe60cQkwHFFXs77u`, `councillor-mimo` = `ses_df21f7d1affeAkYKpZNaNrDgLp`; `councillor-glm` = failed (unavailable preset, retried once).
