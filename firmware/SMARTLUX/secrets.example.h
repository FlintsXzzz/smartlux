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

// Token dashboard (opsional). Jika SMARTLUX_TOKEN tidak didefinisikan, firmware
// memakai 12 digit heksadesimal dari MAC WiFi sebagai token dan mencetaknya di
// Serial beserta URL enroll: http://smartlux.local/?t=<token>
// Token turunan MAC TIDAK aman terhadap orang yang bisa melihat MAC di LAN;
// gunakan hanya untuk demo di jaringan tepercaya. Untuk pemakaian nyata, set
// token sendiri di secrets.h (jangan di-commit).
//
// Aturan token: 1-64 karakter, hanya huruf, angka, '_', '-', dan '.'.
// Jangan pakai Base64 ('+', '/', '=') karena ditolak sebagai nilai cookie.
// Enroll sekali lewat /?t=<token>; setelah itu browser menyimpan cookie
// HttpOnly SameSite=Strict. Token juga tercatat di riwayat browser.
// #define SMARTLUX_TOKEN "ganti-token-panjang-dan-unik"

// Sandi OTA (opt-in). OTA hanya aktif bila define ini ADA dan panjangnya
// >= 8 karakter; kalau tidak, OTA dinonaktifkan (pesan di Serial). Token
// dashboard turunan MAC tidak dipakai karena mudah dihitung orang di LAN.
// Pakai sandi unik yang kuat dan jangan di-commit.
// #define SMARTLUX_OTA_PASS "ganti-sandi-ota-panjang"

// MQTT + Home Assistant discovery (opt-in, R6). Kopi blok di bawah ke secrets.h
// lalu isi alamat broker. Tanpa SMARTLUX_ENABLE_MQTT, kode MQTT tidak dikompilasi
// sama sekali: firmware dasar tetap byte-for-byte seperti sebelumnya.
//
// WiFiClient ESP32 TIDAK menyelesaikan nama ".local" mDNS. Pakai alamat IP
// broker di VLAN tepercaya/IoT atau nama DNS yang bisa di-resolve router.
// Broker hanya lokal — JANGAN pakai broker cloud untuk data okupansi kelas.
// #define SMARTLUX_ENABLE_MQTT
// #define MQTT_BROKER "192.168.1.10"
// #define MQTT_PORT 1883                       // opsional, default 1883
// #define MQTT_USER "smartlux-r6"             // opsional; harus punya MQTT_PASS
// #define MQTT_PASS "sandi-akun-broker-unik"  // opsional; jangan pakai token/OTA
// #define MQTT_BASE_TOPIC "smartlux"          // opsional, default smartlux
// #define MQTT_DISCOVERY_PREFIX "homeassistant" // opsional, default homeassistant
//
// Akun broker sebaiknya haknya terbatas (ACL per perangkat):
//   readwrite smartlux/<id>/#
//   write homeassistant/+/smartlux_<id>/+/#
// "<id>" = 6 digit heksadesimal dari 24 bit bawah MAC, dicetak di Serial saat boot.
// Sandi ini ikut ter-compile ke .bin (bisa dibaca dengan strings); ganti saat
// perangkat dipensiunkan. Jangan pakai SMARTLUX_TOKEN atau SMARTLUX_OTA_PASS.
