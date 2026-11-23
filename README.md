# SMARTLUX

Lampu otomatis hemat energi untuk praktikum fisika — Kelompok 2, SMAN CMBBS (2026/2027).

SMARTLUX menyalakan lampu hanya saat ruangan **gelap dan ada gerakan**, lalu mematikannya setelah jeda. Sebuah ESP32 membaca LDR dan PIR, mengukur energi lewat INA219, menampilkan status di OLED, dan menyajikan dashboard web lokal melalui WiFi. Proyek ini mencakup proposal praktikum, firmware, desain casing 3D, dan harness evaluasi tampilan.

## Isi repository

| Path | Isi |
| --- | --- |
| `proposal_SMARTLUX_Kelompok_2.md` | Proposal praktikum (BAB I–III, K3, daftar pustaka, lampiran, skema koneksi v2). |
| `firmware/SMARTLUX/` | Firmware ESP32, wiring, endpoint API, format CSV, mesin status. Lihat [README firmware](firmware/SMARTLUX/README.md). |
| `hardware/enclosure/` | Model casing OpenSCAD parametris dan panduan cetak/perakitan. Lihat [README casing](hardware/enclosure/README.md). |
| `gan-harness/` | Brief desain, rubrik, dan kandidat dashboard yang dievaluasi. |
| `docs/superpowers/` | Spesifikasi dan rencana implementasi (mesin status, API hardening, aksesibilitas). |

## Komponen utama

- **ESP32 DevKit** — GPIO34 (LDR/ADC1), GPIO27 (PIR OUT), GPIO26 (gate MOSFET), GPIO21/22 (I²C SDA/SCL), GPIO32 (tombol override, `INPUT_PULLUP` aktif LOW).
- **Sensor & aktuator** — LDR + divider 10 kΩ, PIR HC-SR501, MOSFET logic-level sebagai low-side switch untuk LED DC 5 V.
- **Tampilan & ukur** — OLED SSD1306 0x3C, INA219 0x40 (high-side di rel 5 V, shunt 0,1 Ω).
- **Kontrol** — tombol override (short press siklus `AUTO → NYALA → MATI`, long press kembali ke `AUTO`) dan dashboard web.

Skema koneksi lengkap ada di Gambar 2 dalam proposal dan di [README firmware](firmware/SMARTLUX/README.md#1-wiring).

## Mulai cepat

```bash
# 1. Kredensial WiFi (tidak di-commit)
cp firmware/SMARTLUX/secrets.example.h firmware/SMARTLUX/secrets.h
#    lalu isi WIFI_SSID dan WIFI_PASS (2,4 GHz)

# 2. Build (pilih salah satu)
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX
arduino-cli compile --profile smartlux firmware/SMARTLUX   # versi terkunci (sketch.yaml)
pio run -d firmware/SMARTLUX

# 3. Upload & monitor
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32 firmware/SMARTLUX
pio run -d firmware/SMARTLUX -t upload
pio device monitor -d firmware/SMARTLUX -b 115200
```

Tanpa `secrets.h`, setelah 20 detik gagal terhubung firmware membuka AP `SMARTLUX-XXXX` dengan sandi `smartlux-xxxx` (diturunkan dari MAC, dicetak di Serial). Buka `http://192.168.4.1` atau `http://smartlux.local`. Jika mDNS `.local` gagal (jaringan sekolah sering memblokir multicast), gunakan alamat IP yang tampil di Serial/OLED. Lihat detail lengkap di [README firmware](firmware/SMARTLUX/README.md#4-wifi--dashboard).

## Dashboard

Halaman tunggal inline (tanpa CDN), polling 1 detik, menampilkan kondisi lampu, kecerahan, tegangan/arus/daya, energi, dan penghematan terhadap lampu yang selalu nyala. Tiga tombol mode (`AUTO`/`NYALA`/`MATI`) dan reset energi tersedia di halaman.

| Endpoint | Fungsi |
| --- | --- |
| `GET /` | Dashboard |
| `GET /api/status` | JSON status (23 kunci) |
| `GET /api/override?mode=auto\|on\|off` | Paksa mode |
| `GET /api/reset` | Nol-kan energi terhitung |

## Keselamatan

- GPIO34/32/21/22 hanya **3,3 V**; jangan masukkan 5 V ke pin ini.
- Saat pengukuran pakai **satu** sumber 5 V teregulasi dan **cabut USB pemrograman** — jangan dua sumber sekaligus; jangan pernah >12 V atau PLN 220 V.
- Semua ground disatukan. Ukur tegangan OUT PIR (≤ 3,6 V) sebelum menyambung ke GPIO27.
- Kalibrasi ambang ADC, lux, dan daya lampu **wajib** dilakukan terhadap multimeter/INA219 sebelum data dipakai (lihat [bagian kalibrasi](firmware/SMARTLUX/README.md#6-kalibrasi-wajib-sebelum-dipakai)).

## Status pengembangan

Firmware lolos kompilasi dengan Arduino CLI dan PlatformIO. Pengujian fisik berikut masih menunggu prototipe nyata: kalibrasi LDR/INA219 terhadap multimeter, uji dashboard via LAN, cetak dan dry-fit casing, serta verifikasi skema terhadap rangkaian akhir sebelum diberi daya.

## Keamanan

Lihat [kebijakan keamanan](.github/SECURITY.md) untuk cara melaporkan kerentanan. Perangkat ini ditujukan untuk jaringan lokal yang dipercaya. Kredensial WiFi ada di `firmware/SMARTLUX/secrets.h`, yang di-ignore git.

Aksi dashboard (`/api/override`, `/api/reset`) dilindungi token: daftar sekali lewat `http://smartlux.local/?t=<token>`, lalu cookie `smartlux_auth` dipakai browser. `/api/status` tetap bisa dibaca tanpa token. Token turunan MAC hanya untuk demo di LAN yang dipercaya; set `SMARTLUX_TOKEN` di `firmware/SMARTLUX/secrets.h` untuk pemakaian lain.

## Lisensi

- Kode (firmware, OpenSCAD, harness) — [MIT](LICENSE).
- Dokumen proposal `proposal_SMARTLUX_Kelompok_2.md` — CC BY 4.0.

