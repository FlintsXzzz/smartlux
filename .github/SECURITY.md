# Security Policy

## Versi yang didukung

| Versi | Didukung |
| --- | --- |
| `main` (terbaru) | Ya |
| Rilis lama | Tidak |

## Melaporkan kerentanan

Jangan membuka Issue publik untuk masalah keamanan. Gunakan pelaporan
kerentanan privat GitHub:

[Laporkan kerentanan](https://github.com/FlintsXzzz/smartlux/security/advisories/new)

Sertakan langkah reproduksi dan dampaknya. Kami berusaha membalas dalam 48 jam
dan menutup masalah yang terkonfirmasi dalam 14 hari.

## Catatan keamanan proyek ini

SMARTLUX adalah perangkat IoT di jaringan lokal, bukan layanan internet.

- Kredensial WiFi berada di `firmware/SMARTLUX/secrets.h`, yang di-ignore git
  dan tidak pernah di-commit. Hanya `secrets.example.h` yang masuk repositori.
- Sandi access point (AP) cadangan tidak lagi berupa nilai tetap di kode: nilainya
  diambil dari `SMARTLUX_AP_PASS` di `secrets.h`, atau diturunkan dari MAC
  perangkat (`smartlux-xxxx`) dan dicetak di Serial.
- Dashboard web belum memakai autentikasi. Perangkat ini hanya untuk jaringan
  lokal yang dipercaya (laboratorium sekolah).
- Jangan mengekspos port 80 perangkat ke internet publik.
- Firmware belum memakai TLS. Bila dashboard akan dijangkau di luar LAN,
  tambahkan HTTPS lebih dulu.
