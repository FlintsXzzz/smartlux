# Repo Tests — SMARTLUX

Tes repo ini adalah **guard invarian statis** untuk proposal, firmware, dan
kebersihan berkas. Mereka memakai **Node.js built-in test runner** (`node:test`
dengan `node:assert/strict`) — tanpa npm package, tanpa dependensi baru.

Tes ini **bukan** tes hardware. Mereka memeriksa struktur dan integritas berkas
yang bisa dilindungi otomatis. Validasi hardware (kalibrasi LDR/INA219,
uji dashboard via LAN, cetak casing, dry-fit) tetap membutuhkan prototipe nyata.

## Menjalankan tes repo

Dari root repo:

```bash
export PATH="$HOME/.local/node/bin:$PATH"
node --test tests/repo/*.test.js
```

Catatan: tanpa `package.json`, file `.js` di repo ini dibaca sebagai CommonJS, jadi
`require('node:test')` dan `require('node:assert/strict')` sudah cukup.

Node versi 24 membaca test file melalui glob, jadi folder test disarankan
diberi jalur glob, misalnya:

```bash
node --test "tests/repo/**/*.test.js"
```

atau lewat shell glob:

```bash
node --test tests/repo/*.test.js
```

## Tes native firmware (logik murni)

Tes unit untuk `firmware/SMARTLUX/logic.h` (logika lampu, mode override, LDR
histeresis, energi, penghematan) memakai PlatformIO + Unity di host:

```bash
pio test -d tests/native -e native
```

## Build firmware ESP32

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX
# atau versi terkunci:
arduino-cli compile --profile smartlux firmware/SMARTLUX

pio run -d firmware/SMARTLUX
```

## Daftar tes

| File | Isi diproteksi |
| --- | --- |
| `tests/repo/proposal.test.js` | Struktur proposal: dua baris base64 PNG, heading BAB I–III, placeholder `[Isi]`, tabel markdown. |
| `tests/repo/firmware.test.js` | Payload `INDEX_HTML` ≤ 8192 byte, tidak ada baris JS di kolom 0, pin map GPIO, route HTTP, `logic.h` tanpa Arduino header, status `secrets`. |
| `tests/repo/hygiene.test.js` | `.gitignore` (secrets/env/build), tidak ada file `*.stl` ter-track, berkas wajib ada, `platform = native`. |
| `tests/repo/mqtt.test.js` | Opt-in MQTT (R6): guard saat `SMARTLUX_ENABLE_MQTT` absen, `PubSubClient@2.8`, `setBufferSize(1536)`, `setConnectionTimeout(2000)`, LWT retained, 9 entitas discovery, 4 env CI di bawah 90% flash, dan kunci `value_json` selalu ada di `/api/status`. |

## Status

Tes repo ini melindungi *invarian dokumentasi dan wiring* yang sering rusak saat
review. Mereka tidak menggantikan tes unit native (`tests/native`) dan tidak
menggantikan pengujian fisik pada prototipe.
