# Changelog

Semua perubahan penting pada SMARTLUX dicatat di sini.
Format mengikuti [Keep a Changelog](https://keepachangelog.com/id/1.1.0/),
dan proyek memakai [Semantic Versioning](https://semver.org/lang/id/).

## [Unreleased]

### Added
- Logika keputusan murni diekstrak ke `firmware/SMARTLUX/logic.h` dan diuji dengan
  Unity di `tests/native/` (`pio test -d tests/native -e native`, 12/12 lulus);
  CI menjalankan job `native-tests` terpisah.

### Fixed
- `luxEstimate()` kini meng-clamp ADC ke 0..4095 seperti `lightPercent()`, sehingga
  pembacaan di luar rentang tidak menghasilkan `NaN` pada `lux_est`.
- `static_assert` mengunci `enum Mode` ke 0/1/2 sesuai asumsi `logic.h`.

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

### Security
- Kredensial WiFi disimpan di `secrets.h` yang diabaikan git; kata sandi AP
  berasal dari `SMARTLUX_AP_PASS` atau diturunkan dari MAC (`smartlux-xxxx`).
- Secret scanning, push protection, pembaruan keamanan Dependabot, dan
  pelaporan kerentanan privat diaktifkan pada repositori publik.
