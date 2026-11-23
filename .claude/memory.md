# SMARTLUX — durable project memory

Compact, checked-in state for any agent picking this repo up cold.
`AGENTS.md` is the binding rulebook; this file is the running state log.
Update it when a decision or verification result changes — not per edit.

## What ships

| Artifact | Path | Status |
| --- | --- | --- |
| Proposal (deliverable, Indonesian) | `proposal_SMARTLUX_Kelompok_2.md` | ~755 lines, complete draft; `[Isi]` / logo placeholders intentionally unfilled |
| Firmware | `firmware/SMARTLUX/SMARTLUX.ino` + `logic.h` | compile-verified, no hardware run yet |
| Native unit tests | `tests/native/` (PlatformIO `platform = native`, Unity) | 28/28 pass |
| Repo invariant tests | `tests/repo/` (`node --test`) | static/document guards |
| Enclosure | `hardware/enclosure/smartlux_case.scad` + `params.csv` | parametric only; OpenSCAD not installed here, no STL rendered |
| Dashboard design winner | `gan-harness/iterations/03/index.html` | ported into `INDEX_HTML` |

## Verification commands (the real gates)

```bash
node --test tests/repo/                                   # document + firmware + hygiene invariants
pio test -d tests/native -e native                        # pure logic unit tests
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX
pio run -d firmware/SMARTLUX                              # needs src_dir = . (already set)
```

Use native WSL node (`export PATH="$HOME/.local/node/bin:$PATH"`, v24.x).
`~/.local/bin/node` is a Windows shim and must not be used for MCP or tooling.
There is no npm/pip project here — do not invent build commands.

## Binding hardware facts

- GPIO34 LDR divider (ADC1, 11 dB, 3.3 V max — never 5 V), GPIO27 PIR OUT,
  GPIO26 MOSFET gate, I²C SDA 21 / SCL 22 (OLED 0x3C + INA219 0x40),
  override button GPIO32 (`INPUT_PULLUP`, active LOW, 50 ms debounce).
- Lamp on only when dark AND motion; delay-off after last motion.
- Logic-level N-MOSFET low-side switch, all grounds common.
- One regulated 5 V source during measurement; disconnect the laptop USB.
  Never >12 V, never mains.

## Non-obvious traps (learned the hard way)

1. **Two base64 PNG lines** inside the proposal (~177 KB + ~263 KB). Never
   Write the file wholesale, never run a formatter over it. Targeted edits only;
   re-locate the lines after structural edits instead of trusting line numbers.
2. **ino2cpp**: PlatformIO treats column-0 JS `function` lines inside
   `INDEX_HTML` as C++ prototypes. Those six lines are indented one space on
   purpose — do not "fix" the indentation.
3. **`INDEX_HTML` budget 8192 bytes.** Currently 8,175 — only 17 bytes of
   headroom. Any dashboard addition must be paid for by a removal.
4. **`secrets.h` is git-ignored and absent**; the PlatformIO build therefore
   warns that `g_haveCreds` is unused. That warning is expected.
5. **Specialist subagents on free tiers fail** ("Free usage limit reached").
   Do not wait on a dead lane — apply fully specified edits directly and use
   `code-reviewer` for a read-only audit.

## Decision log

- 2026-11-20 — research revisions (ADC calibration, PIR warm-up, MOSFET gate
  network, energy decomposition, statistics, K3, 16 references).
- 2026-11-21 — v2 features: OLED, dashboard, override state machine, enclosure.
  11 explicit state machines named; dashboard simplified, a11y-fixed, API
  rate-limited (300 ms) + CSP; wiring schematic redrawn.
- 2026-11-23 — repo hardening: MIT + CC BY 4.0, SECURITY.md, `sketch.yaml`
  profile (no `default_profile`), `master` → `main`, repo public, CI with an
  `INDEX_HTML` guard job, issue/PR templates, CODEOWNERS, Dependabot,
  `CHANGELOG.md`, `docs/calibration.md`, tag `v0.1.0`.
- 2026-11-23 — pure logic extracted to `logic.h` + native Unity tests; C++
  review fix restored the ADC clamp in `luxEstimate()`; button debounce
  rewritten TDD-first (`stableButtonState`), `logic.h` at 100% line coverage.
- 2026-11-23 — dashboard cookie auth for `/api/override` and `/api/reset`
  (`SMARTLUX_TOKEN`, else MAC-derived token printed to Serial). `/api/status`
  stays public; the dashboard HTML/JS is byte-identical.
- 2026-11-23 — harness audit (22/39) remediated: `.claude/` project surface
  (settings + PreToolUse guard hook), checked-in repo invariant tests, `.env`
  ignore rules, this memory file.
- 2026-11-23 — open-source comparison (TinyFish run
  `01a2083e-cab8-7e7f-bac4-be00dabbe42b`) adopted R1–R5 + OTA: ADC median-of-5,
  two-level LEDC dimming (`lamp_pwm` JSON key, `pwm` last CSV column), 30 s TWDT
  + I2C recovery, NVS-persistent energy, `/api/history` + `/api/export` + `/chart`,
  and OTA **opt-in** with `SMARTLUX_OTA_PASS` ≥8 chars (never the MAC token).
  Second `CHART_HTML` raw literal sits after `INDEX_HTML`; CI guards both budgets.

## Still open (needs the physical prototype)

- Empirical LDR threshold + hysteresis calibration (`docs/calibration.md`).
- PIR OUT voltage measurement before wiring GPIO27.
- INA219 energy validation, LAN/dashboard test, enclosure print fit.
- CORS restriction on the dashboard API (deferred: trusted-LAN demo).
