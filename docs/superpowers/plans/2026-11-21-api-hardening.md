# ESP32 HTTP API hardening

Scope: `firmware/SMARTLUX/SMARTLUX.ino` only. No dashboard, endpoint, JSON key, pin or timing change.

## Decisions

| Item | Choice |
|---|---|
| J3 rate limiting | Shared 300 ms gate on the two mutating routes, `429` + `Retry-After: 1`. Reads untouched. |
| J4 mDNS | Tear down and re-register the responder on every interface change, plus a 10 s retry. |
| J6 CSP | `default-src 'none'` with `'unsafe-inline'` for the one style and one script block. |

Rejected: token bucket on all routes (a burst would flash the dashboard's `Terputus`), mDNS docs-only (the recovery path added on 2026-11-21 leaves the responder stale), sha256 hash pin (every dashboard edit would silently kill the page).

## Changes

1. `actionAllowed()` gate before `handleOverride` / `handleReset`, checked after `sameOrigin()` so 403 still wins over 429.
2. `restartMdns()` replacing the bare `startMdns()` at the two call sites, plus a retry branch in `pollNetwork()` for the `MDNS.end()`/`MDNS.begin()` race.
3. A `PROGMEM` CSP constant sent from `handleRoot()` only; CSP is enforced on documents, not on JSON.

## Review pass

`code-reviewer` rated all five hunks clean apart from two findings, both fixed:

- A 429 is a resolved `fetch()`, so the dashboard's mode buttons and reset silently dropped a second action inside the 300 ms window. Added `act()`, which retries once after 350 ms. No visual or structural change; the page is 8,122 bytes, 70 under the 8 KB PROGMEM budget.
- A partial `MDNS.begin()` failure left the subsystem initialized while the flag stayed false, so the retry could never recover. `startMdns()` now stamps `g_mdnsRetryMs` and calls `MDNS.end()` on failure. The action gate also moved after argument validation so malformed input does not consume the slot.

## Verification

- `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX` PASS: 1,026,968 B flash (78 %), 51,360 B RAM (15 %).
- Static check: four `server.on()` routes, 23 JSON keys, and the CSP + `Retry-After` headers all present. The served page differs from `gan-harness/iterations/03/index.html` in exactly four JS lines.
- Not run: live `curl` curve check and physical LAN tests. No hardware or server is attached to this session.

## Out of scope

Authentication, POST-for-mutation, TLS, physical LAN tests.
