// secrets.example.h - salin menjadi secrets.h lalu isi kredensial.
// secrets.h TIDAK boleh di-commit (berisi password WiFi).
//
//   cp firmware/SMARTLUX/secrets.example.h firmware/SMARTLUX/secrets.h
//
// Jika secrets.h tidak ada (atau nilainya masih placeholder), firmware
// otomatis menjalankan mode Access Point:
//   SSID     : SMARTLUX-XXXX  (XXXX = 4 digit terakhir MAC)
//   Password : smartlux-xxxx  (diturunkan dari MAC, dicetak di Serial)
//   URL      : http://192.168.4.1  atau  http://smartlux.local

#pragma once

// Isi dengan kredensial WiFi rumah/sekolah. Hanya 2,4 GHz yang didukung ESP32.
#define WIFI_SSID "GANTI_SSID"
#define WIFI_PASS "GANTI_PASSWORD"

// Password AP fallback. Baris ini opsional: kalau dibiarkan komentar, firmware
// menurunkan sandi dari MAC perangkat sehingga tidak ada sandi tetap di repo.
// Kalau diaktifkan, pakai minimal 8 karakter untuk WPA2 dan jangan di-commit.
// #define SMARTLUX_AP_PASS "ganti-sandi-ap"
