# SMARTLUX — Explicit State Machines (design)

Date: 2026-11-21

## Intent
The firmware already contains eleven state machines, but most are encoded as loose `millis()` timers, booleans and sentinel values. That is hard to read (DX), invisible to the operator (UX) and hard for an agent to reason about or change safely (AX). Make every machine's states and transitions explicit and observable **without changing any behavior**.

## Goals
- Name each machine's states with an `enum class`/`enum` and keep its state in one typed variable.
- Route every state change through one small transition function per machine.
- Append state fields to `/api/status` JSON and to the CSV serial log; never remove or rename existing fields.
- Document state tables and transitions in `firmware/SMARTLUX/README.md`.
- Keep the firmware compiling for `esp32:esp32` with the same flash/RAM budget.

## Non-goals / anti-goals
- No timing, threshold, pin, endpoint or transition-semantics changes (including the 800–1500 ms button dead zone).
- No external FSM library or new dependency.
- No new hardware or enclosure changes; no proposal image edits.

## Constraints
- GPIO34/27/26/21/22/32 pin map; ADC1 11 dB; no 5 V on GPIO; no `delay()` in `loop()`.
- All timings unchanged: warm-up 60 s, delay-off 30 s, sensor 200 ms, power 1 s, OLED pages 3 s / refresh 400 ms, debounce 50 ms, short press 50–800 ms, long press >1500 ms, STA timeout 20 s, reconnect 10 s.
- Thresholds ADC_DARK_ON 1500 / ADC_BRIGHT_OFF 1800 stay example values.
- Existing JSON keys stay byte-identical; new keys are appended.

## Approaches considered
1. **Explicit states, no behavior change (chosen).** Typed state + single transition function per machine, observability added. Best DX/AX/UX per unit of risk; behavior preserved by construction.
2. External FSM library. Rejected: new dependency, larger flash/RAM, worse AX for a sketch this size.
3. Decision-only documentation. Rejected: leaves the code as the source of truth mismatch; no AX gain.

## Concrete refactor
Machines to make explicit in `SMARTLUX.ino`:

| # | Machine | New state type | Transition function |
|---|---------|----------------|---------------------|
| 1 | Mode/override | `Mode` (already exists) | keep `setMode()`/`cycleMode()` as the only writers |
| 2 | Button press classifier | `enum ButtonPress {BTN_IDLE, BTN_PRESSED, BTN_SHORT_FIRED, BTN_LONG_FIRED}` | `pollButton()` calls `buttonTransition()` |
| 3 | PIR warm-up/motion | `enum PirState {PIR_WARMUP, PIR_ARMED, PIR_MOTION, PIR_QUiet}` | `updatePir()` |
| 4 | Light hysteresis | `enum LightState {LIGHT_BRIGHT, LIGHT_DARK}` | `updateLight()` |
| 5 | Network | `NetState` (already exists) | `networkTransition()` used by `setupWifi()`/`pollNetwork()` |
| 6 | OLED pages | `enum OledPage {PAGE_LIGHT, PAGE_POWER, PAGE_ENERGY}` | `advanceOledPage()` |
| 7 | Energy accounting | `enum EnergyPhase {ENERGY_IDLE, ENERGY_LEARNING, ENERGY_READY}` | `updateEnergy()` |
| 8 | I²C presence | `enum DeviceState {DEV_OK, DEV_MISSING}` for OLED and INA219 | set once in `i2cInitState()` |
| 9 | Lamp output | `enum LampState {LAMP_OFF, LAMP_ON, LAMP_FORCED_OFF}` | derived only in `applyControl()` |
| 10 | Dashboard UI | keep JS as-is; add `mode`/`net`/`pir_state`/`devices` badges driven by new JSON keys | no transition rewrite |
| 11 | Boot sequence | `enum BootPhase {BOOT_INIT, BOOT_I2C, BOOT_NET, BOOT_READY}` | `setBootPhase()` calls |

Observability additions (append-only):
- JSON: `pir_state`, `light_state`, `button_state`, `oled_page`, `energy_phase`, `oled_device`, `ina_device`, `lamp_state`, `boot_phase`.
- CSV: add columns after existing ones: `pir_state,light_state,net,oled_page,energy_phase`.
- README: one table per machine listing states, triggers, resulting state and observable field.

## Verification
- `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX` must still succeed.
- `grep` check: timings/constants unchanged; old JSON keys and endpoints unchanged; no `delay(` in loop.
- Diff review that every transition writes state only through its transition function.
- Proposal file untouched; if touched, re-locate base64 lines before and after.
