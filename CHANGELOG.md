# Changelog

Semua perubahan penting pada SMARTLUX dicatat di sini.
Format mengikuti [Keep a Changelog](https://keepachangelog.com/id/1.1.0/),
dan proyek memakai [Semantic Versioning](https://semver.org/lang/id/).

## [Unreleased]

### Added
- Logika keputusan murni diekstrak ke `firmware/SMARTLUX/logic.h` dan diuji dengan
  Unity di `tests/native/` (`pio test -d tests/native -e native`, 33/33 lulus);
  CI menjalankan job `native-tests` terpisah.
- `stableButtonState()` di `logic.h` mendebounce tombol override secara murni;
  `pollButton()` memakainya tanpa mengubah timing 50 ms. Empat tes baru
  (tepi tekan, tepi lepas, bounce, jendela nol) ditulis lebih dulu (RED).
- Filter median 5 sampel untuk ADC LDR (`adcMedian5()` di `logic.h`) menekan
  spike pembacaan tanpa menambah komponen; delapan tes native ditulis lebih
  dulu (RED).
- Peredupan dua tingkat memakai PWM LEDC 1 kHz/8 bit pada GPIO26: duty penuh
  saat gerakan, 128 (~50%) pada 15 detik terakhir masa tunda, lalu 0. Helper
  murni `lampBrightness()` menghasilkan nilai tersebut; status `lamp_pwm`
  ditambahkan ke `/api/status` dan kolom `pwm` ke akhir CSV.
- Watchdog task ESP32 (30 detik, `trigger_panic=true`, di-reset di `loop()`) dan
  pemulihan bus I²C: `Wire.setTimeOut(100)`, probing yang tetap berjalan walau
  INA219 gagal saat boot, cooldown 30 detik, dan inisialisasi ulang OLED hanya
  bila OLED memang sebelumnya aktif.
- Energi (`energy_wh`, `lamp_energy_wh`, durasi, `P_lamp`) dipersistenkan di NVS
  tiap 60 detik dan dipulihkan saat boot; `/api/reset` menghapusnya.
- Riwayat 60 sampel daya/duty dengan endpoint baru `GET /api/history` (JSON),
  `GET /api/export` (CSV), dan halaman grafik `GET /chart` (PROGMEM).
- BH1750 opsional di I²C 0x23: bila terdeteksi, lux SI (`lux_bh`) menggantikan
  estimasi LDR untuk histeresis (`lightDecisionLux()`, ambang 50/80 lx); tanpa
  sensor, jalur ADC lama tetap dipakai. Lima helper murni baru di `logic.h`
  (`bh1750Lux`, `lightDecisionLux`, `validDelaySeconds`, `pwmFrequencyAllowed`,
  `savingsMarginPct`) ditulis RED-first; suite native kini 33/33.
- Delay-off dapat diubah 5–200 detik saat berjalan lewat `GET /api/config` dan
  halaman `/chart` (default 30 detik, tersimpan di NVS). Frekuensi PWM dapat
  dipilih 100/500/1000/5000 Hz untuk eksperimen kedip LED.
- `/api/status` menambah `lux_bh`, `delay_s`, `pwm_hz`, dan `savings_margin_pp`
  (margin galat penghematan dari asumsi INA219 ±2% dan baseline ±5%); `/chart`
  menampilkan lux, penghematan ± margin, label baseline terukur, serta kontrol
  delay/PWM.
- OTA via ArduinoOTA (port 8266) bersifat opt-in: hanya aktif dengan
  `SMARTLUX_OTA_PASS` minimal 8 karakter di `secrets.h`; tanpa itu OTA mati.
  Token dashboard turunan MAC tidak dipakai sebagai sandi OTA.
- CI kini memeriksa anggaran PROGMEM untuk dua halaman (`INDEX_HTML` 8.192 byte,
  `CHART_HTML` 4.096 byte) dan tetap menolak baris `function` di kolom pertama.
- MQTT + Home Assistant discovery (R6) bersifat opt-in lewat `SMARTLUX_ENABLE_MQTT`
  di `secrets.h`. PubSubClient@2.8 dikunci, buffer 1536 byte, batas koneksi TCP
  2000 ms memakai `setConnectionTimeout()`, LWT retained `offline`, sembilan
  entity read-only dengan `unique_id` dari 6 hex MAC, telemetri JSON `/api/status`
  tiap 5 detik, dan reconnect non-blocking dengan backoff 5–30 detik. Tanpa define,
  firmware dasar tidak berubah.
- `buildStatusJson()` diekstrak dari `handleStatus()` sehingga HTTP dan MQTT
  memakai satu formatter JSON yang sama (28 kunci). CI membangun empat varian
  (dasar, MQTT, OTA, MQTT+OTA) dengan batas 90% flash dan menjaga pertumbuhan
  build dasar ≤100 byte. Sepuluh helper murni baru di `logic.h` ditulis RED-first;
  suite native kini 43/43 dan tes repo 31/31.

### Fixed
- `luxEstimate()` kini meng-clamp ADC ke 0..4095 seperti `lightPercent()`, sehingga
  pembacaan di luar rentang tidak menghasilkan `NaN` pada `lux_est`.
- `static_assert` mengunci `enum Mode` ke 0/1/2 sesuai asumsi `logic.h`.
- Audit keamanan: OTA tidak lagi memakai token turunan MAC (eskalasi ke unggah
  firmware sembarangan); pemulihan I²C tidak lagi mengunci diri bila INA219
  gagal; `handleHistory()` memeriksa nilai `snprintf` sebelum menambah offset;
  `esp_task_wdt_config_t` di-zero-init dan `/chart` ikut memakai header CSP.

## [0.1.0] - 2026-11-23

Rilis pertama: firmware terkompilasi, dashboard, enclosure, dan dokumen proposal.

### Added
- Firmware ESP32 (`firmware/SMARTLUX/SMARTLUX.ino`): kontrol lampu "gelap DAN gerak"
  dengan delay-off, override tombol (tekan singkat 50–800 ms, tekan lama >1500 ms),
  OLED SSD1306 (0x3C), pengukuran INA219 (0x40), dan 11 mesin status eksplisit.
- Dashboard lokal dari `INDEX_HTML` di PROGMEM dengan rute `GET /`,
  `GET /api/status`, `GET /api/override?mode=auto|on|off`, `GET /api/reset`;
  polling 1 dtk, mDNS `smartlux.local`, fallback AP `SMARTLUX-XXXX`.
- Hardening API: batas satu aksi per 300 ms (429 + retry di dashboard),
  CSP `default-src 'none'`, dan pemeriksaan same-origin pada rute yang mengubah status.
- Enclosure parametrik OpenSCAD (`hardware/enclosure/`).
- Dokumen proposal dan skema v2 (bus I²C OLED/INA219 dan tombol override).
- Infrastruktur repo: MIT `LICENSE`, `.github/SECURITY.md`, `sketch.yaml`
  (profil `smartlux` terkunci tanpa `default_profile`), workflow CI build,
  template issue/PR, `CODEOWNERS`, dan `dependabot.yml` untuk GitHub Actions.
- Autentikasi token untuk aksi dashboard (`/api/override`, `/api/reset`): token dari `SMARTLUX_TOKEN` atau turunan MAC, pendaftaran sekali via `/?t=<token>` dengan cookie `smartlux_auth` (HttpOnly, SameSite=Strict, Path=/), helper murni di `logic.h`, serta validasi token saat boot. `/api/status` tetap publik. 20 tes native.

### Security

- Token dashboard turunan MAC dapat dihitung dari MAC WiFi yang terlihat di LAN; gunakan `SMARTLUX_TOKEN` di luar demo. Token pendaftaran tercatat di riwayat browser; halaman sudah `Cache-Control: no-store`.
- Kredensial WiFi disimpan di `secrets.h` yang diabaikan git; kata sandi AP
  berasal dari `SMARTLUX_AP_PASS` atau diturunkan dari MAC (`smartlux-xxxx`).
- Secret scanning, push protection, pembaruan keamanan Dependabot, dan
  pelaporan kerentanan privat diaktifkan pada repositori publik.
