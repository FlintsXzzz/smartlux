# SMARTLUX Explicit State Machines Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:executing-plans. Steps use `- [ ]`.

**Goal:** Make all 11 implicit firmware state machines explicit and observable without changing behavior.

**Architecture:** Keep the existing 800-line sketch and all timing logic. Add named enums plus one transition helper per machine; every state write goes through that helper. Append (never replace) telemetry fields to JSON and CSV, and document each machine in the firmware README.

**Tech Stack:** Arduino-ESP32 C++, `arduino-cli` 1.6.0, ESP32 core 3.3.12.

**Spec:** `docs/superpowers/specs/2026-11-21-state-machines-design.md`

## Global Constraints
- Pins unchanged: 34 LDR (ADC1 11 dB), 27 PIR, 26 MOSFET, 21 SDA, 22 SCL, 32 button.
- Timings unchanged: 60000, 30000, 200, 1000, 3000, 400, 50, 800, 1500, 20000, 10000 ms.
- Thresholds unchanged: ADC_DARK_ON 1500, ADC_BRIGHT_OFF 1800.
- Existing JSON keys and endpoints unchanged; telemetry keys appended only.
- No `delay()` in `loop()`. Proposal `.md` untouched.

## Review Focus
- Button 800–1500 ms dead zone must remain a no-op; transition helper must not fire an extra mode change.
- PIR warm-up must still force lamp OFF and reset motion window sentinel.
- Network AP fallback/reconnect timing must not drift.
- Energy integral and learned lamp power must keep identical values.
- JSON must keep all 14 original keys with the same names.

---

### Task 1: Add state types and transition helpers
**Files:** Modify `firmware/SMARTLUX/SMARTLUX.ino` (globals/enums area).
- [ ] Add enums: `ButtonPress`, `PirState`, `LightState`, `OledPage`, `EnergyPhase`, `DeviceState`, `LampState`, `BootPhase` with the exact names from the spec.
- [ ] Add one `static void xxxTransition(Xxx s)` per enum that assigns the matching global state variable.
- [ ] Verify: `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX` succeeds.

### Task 2: Route all writes through transition helpers
**Files:** Modify `firmware/SMARTLUX/SMARTLUX.ino` (`applyControl`, `setMode`, `pollButton`, `sampleSensors`, `samplePower`, OLED poll, network setup/poll, I²C init, `setup`).
- [ ] Replace direct writes to state variables with the helper calls; leave all timing/condition expressions byte-identical.
- [ ] Keep `g_oledPage` int for rendering; mirror into `OledPage` via helper.
- [ ] Verify: compile succeeds; `grep -n 'delay(' firmware/SMARTLUX/SMARTLUX.ino` returns no hits.

### Task 3: Append telemetry
**Files:** Modify `firmware/SMARTLUX/SMARTLUX.ino` (`handleStatus`, CSV header + `loop` printf).
- [ ] Append JSON keys: pir_state, light_state, button_state, oled_page, energy_phase, oled_device, ina_device, lamp_state, boot_phase; enlarge snprintf buffer.
- [ ] Append CSV columns: pir_state,light_state,net,oled_page,energy_phase and matching printf args.
- [ ] Verify: compile succeeds; all 14 original JSON keys still present.

### Task 4: Document and validate
**Files:** Modify `firmware/SMARTLUX/README.md`.
- [ ] Add one state table per machine (states, trigger, resulting state, telemetry field).
- [ ] Verify: full compile after all edits; diff review that no timing/threshold/pin/endpoint changed; proposal base64 lines still valid if touched (leave untouched).
