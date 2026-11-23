# SMARTLUX v2 — Firmware ESP32

Firmware lampu otomatis hemat energi: LDR + PIR, OLED, INA219, tombol override,
dan dashboard web lokal. Referensi desain: `proposal_SMARTLUX_Kelompok_2.md`.

> **Keselamatan:** GPIO34/32/21/22 hanya 3,3 V. Semua ground common.
> INA219 hanya high-side di rel 5 V. Saat pengukuran pakai **satu** sumber 5 V
> dan **cabut USB pemrograman** — jangan dua sumber sekaligus.

## 1. Wiring

| Fungsi | ESP32 | Tujuan | Catatan |
|---|---|---|---|
| LDR divider | GPIO34 | titik tengah divider LDR–resistor | ADC1, 11 dB, maks 3,3 V |
| PIR OUT | GPIO27 | OUT HC-SR501 | ukur dulu OUT ≤ 3,6 V |
| Gate MOSFET | GPIO26 | 220–470 Ω → gate, 10 kΩ gate–GND | logic-level N-MOSFET |
| OLED SDA | GPIO21 | SSD1306 SDA | 3,3 V |
| OLED SCL | GPIO22 | SSD1306 SCL | 3,3 V, 100 kHz |
| OLED VCC/GND | 3V3/GND | SSD1306 VCC/GND | **bukan 5 V** |
| INA219 SDA/SCL | GPIO21/22 | INA219 | alamat 0x40 |
| INA219 VIN+ | — | suplai 5 V (sisi sumber) | high-side |
| INA219 VIN− | — | beban 5 V (ESP32 5V, PIR, rel LED) | high-side |
| INA219 VCC/GND | 3V3/GND | — | logika 3,3 V |
| Tombol | GPIO32 | kaki lain → GND | INPUT_PULLUP, aktif LOW |

Alamat I2C: OLED `0x3C`, INA219 `0x40`. Firmware memindai bus saat boot dan
tetap jalan (tanpa OLED/INA) bila perangkat tidak ditemukan.

## 2. Instalasi library

arduino-cli:

```bash
arduino-cli lib install "Adafruit SSD1306" "Adafruit GFX Library" "Adafruit INA219"
```

PlatformIO (otomatis dari `platformio.ini`):

```bash
pio pkg install -d firmware/SMARTLUX
```

## 3. Build & upload

arduino-cli:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 firmware/SMARTLUX
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

PlatformIO:

```bash
pio run -d firmware/SMARTLUX
pio run -d firmware/SMARTLUX -t upload
pio device monitor -d firmware/SMARTLUX -b 115200
```

## 4. WiFi & dashboard

1. `cp firmware/SMARTLUX/secrets.example.h firmware/SMARTLUX/secrets.h`
2. Isi `WIFI_SSID` / `WIFI_PASS` (2,4 GHz). Jangan commit `secrets.h`.
3. Tanpa `secrets.h` (atau masih placeholder), setelah gagal STA 20 detik
   firmware membuka AP: SSID `SMARTLUX-XXXX`, password `smartlux-xxxx`.
   Sandi itu diturunkan dari MAC dan dicetak di Serial. Untuk sandi sendiri,
   aktifkan `SMARTLUX_AP_PASS` di `secrets.h`.
4. Selama AP hidup, firmware mencoba STA lagi tiap 60 detik; kalau berhasil,
   AP ditutup dan perangkat pindah ke IP dari router.

Akses: `http://smartlux.local` (mDNS) atau IP yang tampil di Serial/OLED.
Responder mDNS didaftarkan ulang setiap perangkat berpindah AP/STA. Jaringan
sekolah yang memblokir multicast sering gagal menerjemahkan `.local`; pakai IP.

| Endpoint | Fungsi |
|---|---|
| `GET /` | dashboard (HTML/CSS/JS inline, tanpa CDN), polling 1 dtk |
| `GET /api/status` | JSON: `light_adc, light_pct, lux_est, pir, lamp, mode, voltage_v, current_ma, power_mw, energy_wh, lamp_energy_wh, savings_pct, uptime_s, ip` + status tambahan `pir_state, light_state, button_state, oled_page, energy_phase, oled_device, ina_device, lamp_state, boot_phase, lamp_pwm` |
| `GET /api/override?mode=auto\|on\|off` | paksa AUTO / NYALA / MATI |
| `GET /api/reset` | nol-kan energi terhitung, riwayat, dan nilai tersimpan di NVS (daya lampu hasil belajar ikut dihapus) |
| `GET /api/history` | JSON 60 sampel terakhir (`t` detik, `mw`, `pwm`) untuk grafik; publik, hanya baca |
| `GET /api/export` | unduh CSV `t_s,mw,pwm` dari riwayat yang sama |
| `GET /chart` | halaman grafik daya dan duty lampu (PROGMEM, tanpa CDN) |

`/api/override` dan `/api/reset` dibatasi satu aksi per 300 ms; aksi berikutnya
dalam jendela itu mendapat `429` dengan `Retry-After: 1`. `/api/status` tidak
dibatasi supaya polling 1 detik tidak terpengaruh. Dashboard mengirim header CSP
yang menolak semua muatan dari alamat lain, sementara blok style/script inline
tetap diizinkan. `/api/history`, `/api/export`, dan `/chart` bersifat publik dan
hanya baca, seperti `/api/status`; ketiganya memakai `Cache-Control: no-store`.

Grafik tidak ditautkan dari dashboard karena halaman utama sudah hampir penuh
(8.175 dari 8.192 byte PROGMEM). Buka `http://smartlux.local/chart` langsung;
URL juga dicetak di Serial saat boot. Grafik mengambil data dari `/api/history` tiap 2 detik.

## 5. Tombol override

- Tekan singkat (50–800 ms): `AUTO → NYALA → MATI → AUTO`.
- Tekan lama (> 1500 ms): kembali ke `AUTO`.
- Debounce 50 ms, tanpa `delay()`; mode manual tidak timeout.
- `AUTO`: lampu nyala hanya jika **gelap DAN gerak**, mati 30 detik setelah
  gerakan terakhir.
- Peredupan dua tingkat: GPIO26 memakai PWM LEDC 1 kHz, 8 bit. Saat AUTO, duty
  255 (penuh) ketika gerakan baru saja terdeteksi, turun ke 128 (~50%) pada
  15 detik terakhir masa tunda, lalu 0 saat lampu mati. MANUAL ON selalu 255.
  Duty aktif dapat dibaca lewat kunci `lamp_pwm` dan kolom CSV `pwm`. Peredupan
  bersifat empiris: verifikasi pada LED/Driver yang dipakai (driver arus konstan
  tertentu dapat berkedip); bila bermasalah, kembalikan ke kontrol ON/OFF penuh.

## 6. Kalibrasi (wajib sebelum dipakai)

1. **Ambang cahaya** — `ADC_DARK_ON` / `ADC_BRIGHT_OFF` (1500/1800) hanya
   contoh. Ukur ADC di lokasi saat gelap target dan terang target (lihat CSV
   atau halaman OLED 1), lalu geser ambang ke titik tengah dengan histeresis
   10–20% rentang; jaga `ADC_DARK_ON < ADC_BRIGHT_OFF`.
2. **Lux** — `LUX_FULL_SCALE` hanya estimasi kasar untuk indikator, bukan
   luxmeter terkalibrasi.
3. **Daya lampu** — firmware belajar `P_lamp` saat lampu ON di `MANUAL_ON`
   dan PIR tenang (30 sampel ≈ 30 dtk). Nilai awal `LAMP_DEFAULT_W = 3.0`
   adalah contoh; verifikasi dengan multimeter/INA219.
4. **PIR** — beri warm-up 60 detik (jumper mode H), atur potensiometer sensitivitas
   dan waktu tahan agar tidak double-trigger.
5. **Kalibrasi daya** — bandingkan INA219 dengan beban resistif yang dayanya
   diketahui sebelum mengukur LED; bentuk arus LED dapat menyesatkan pembacaan
   daya. Setelah itu verifikasi ulang pada LED yang dipakai.
6. **Peredupan** — amati transisi 255 → 128 → 0 pada masa tunda AUTO. Pastikan
   tidak ada kedip yang mengganggu dan catat hasilnya. Beberapa LED/driver
   dengan kontrol arus konstan tidak mendukung PWM.

## 7. Format CSV pengukuran

Setiap 1 detik, 115200 baud:

```
t_ms,adc,pir,lamp,mode,v_mv,ma,mw,wh,pir_state,light_state,net,oled_page,energy_phase,pwm
```

Contoh: `125000,820,1,1,auto,4987,312,1554,0.043,motion,bright,sta,light,ready,128`

Kolom `pwm` sengaja ditaruh paling akhir supaya 14 kolom pertama tetap
mempunyai posisi yang sama seperti sebelumnya.

Untuk analisis grafik tanpa Serial, gunakan `/api/history` (JSON) atau
`/api/export` (CSV `t_s,mw,pwm`, 60 sampel terakhir).

## 8. Catatan pengukuran (penting)

- INA219 mengukur **seluruh rel 5 V** (ESP32 + PIR + LED). Karena itu
  `energy_wh` adalah energi sistem, sedangkan `lamp_energy_wh` adalah energi
  sistem saat lampu ON — masih memuat beban dasar ±0,9–1,0 W. Dekomposisi
  daya (P_total − P_quiescent) dibahas di proposal.
- `savings_pct` membandingkan energi lampu terukur dengan baseline
  `P_lamp × durasi` (selalu nyala), sehingga ia mengukur penghematan waktu
  nyala, bukan penghematan daya LED vs lampu konvensional.
- Saat board disuplai USB, arus INA219 hanya membaca cabang beban (USB tidak
  lewat INA219). Untuk data valid: satu sumber 5 V teregulasi, USB dicabut,
  dan gunakan Serial/OLED/dashboard sebagai pencatat (bukan monitor USB).

## 9. Perilaku saat komponen tidak ada

| Kondisi | Perilaku |
|---|---|
| OLED tidak ada | log Serial, kontrol + web tetap jalan |
| INA219 tidak ada | V/I/P = 0, energi tidak bertambah, kontrol tetap jalan |
| WiFi gagal 20 dtk | AP `SMARTLUX-XXXX` / `smartlux-xxxx` (dari MAC) |
| mDNS gagal | akses lewat IP |
| INA219 tidak merespons 3× berturut-turut | bus I²C di-reset (end/begin, timeout 100 ms) lalu OLED/INA219 diinisialisasi ulang; log ke Serial |
| `loop()` macet > 10 dtk | task watchdog ESP32 me-reset board (panic) |

Tidak ada `delay()` di `loop()`; semua periode memakai `millis()`. Pembacaan ADC
juga sudah melalui median 5 sampel supaya spike tunggal tidak mengganti status
cahaya.

## 10. Energi persisten dan riwayat

Nilai `energy_wh`, `lamp_energy_wh`, durasi pengukuran, dan `P_lamp` hasil belajar
disimpan di NVS (`Preferences`, namespace `smartlux`) tiap 60 detik, bukan tiap
detik, agar flash tidak cepat aus. Saat board dinyalakan ulang, nilai tersebut
dipulihkan dan waktu mulai dihitung ulang supaya `savings_pct` tetap konsisten.
Tombol **Reset energi** (`/api/reset`) menghapus nilai NVS dan riwayat grafik.

Riwayat menyimpan 60 sampel terakhir (daya mW dan duty PWM). Data bisa dibaca
sebagai JSON di `/api/history`, diunduh sebagai CSV di `/api/export`, atau
divisualisasikan di `/chart`. Grafik tidak menambah tautan di dashboard utama
karena keterbatasan PROGMEM; buka URL-nya langsung.

## 11. OTA (over-the-air)

Firmware dapat diperbarui lewat jaringan dengan ArduinoOTA (hostname
`smartlux`, port 8266). **OTA hanya aktif bila `SMARTLUX_OTA_PASS` didefinisikan
di `secrets.h` dengan panjang minimal 8 karakter**; tanpa itu OTA dinonaktifkan
dan pesan alasan dicetak di Serial. Token dashboard turunan MAC sengaja **tidak**
dipakai sebagai sandi OTA karena bisa dihitung orang di LAN. Update berjalan di
STA maupun AP, dan `ArduinoOTA.handle()` dipanggil di `loop()` tanpa `delay()`
hanya bila OTA aktif. Contoh:

```bash
pio run -d firmware/SMARTLUX -t upload --upload-port smartlux.local
```

Ganti sandi OTA untuk perangkat yang dipakai di luar demo; jangan biarkan
perangkat dapat diakses dari internet publik. Sandi OTA yang lemah memberi
siapa pun di LAN hak unggah firmware, jadi perlakukan seperti kredensial admin.

## 12. Mesin status eksplisit (observabilitas)

Status yang sebelumnya tersembunyi di variabel dan percabangan kini diberi nama, satu
fungsi transisi, dan telemetri. **Perilaku, pin, ambang, dan waktu tidak berubah**;
yang berubah hanya cara membaca dan menelusuri status. Gunakan `GET /api/status`
(atau CSV Serial) untuk melihat status langsung tanpa membaca kode.

| Mesin | Status | Arti |
| ----- | ------ | ---- |
| Mode (`mode`) | `auto`, `on`, `off` | Kendali lampu: otomatis, paksa nyala, paksa mati. |
| Tombol (`button_state`) | `idle`, `pressed`, `short`, `long` | Klasifikasi tekanan setelah debounce 50 ms; `short` 50–800 ms, `long` >1500 ms. |
| PIR (`pir_state`) | `warmup`, `motion`, `quiet` | Masa pemanasan 60 detik, ada gerakan, atau tidak ada gerakan. |
| Cahaya (`light_state`) | `bright`, `dark` | Histeresis ADC 1800/1500; menggantikan pembacaan `g_dark` secara eksplisit. |
| Lampu (`lamp_state`) | `off`, `on`, `forced_off` | `forced_off` terjadi saat PIR masih warming up walaupun mode meminta nyala. |
| Jaringan (`net`, CSV) | `connecting`, `sta`, `ap` | Proses STA 20 detik, tersambung STA, atau fallback access point. |
| Halaman OLED (`oled_page`) | `light`, `power`, `energy` | Halaman bergilir tiap 3 detik. |
| Energi (`energy_phase`) | `idle`, `learning`, `ready` | Belum ada sampel, sedang mempelajari daya lampu, atau estimasi siap. |
| Perangkat I²C (`oled_device`, `ina_device`) | `ok`, `missing` | Hasil deteksi saat boot; mode degraded tetap berjalan. |
| Boot (`boot_phase`) | `init`, `i2c`, `net`, `ready` | Tahap inisialisasi sampai web server siap. |

Kunci lama di `/api/status` dan empat endpoint awal tidak dihapus; sepuluh kunci
status baru (`lamp_pwm` termasuk) ditambahkan di bagian akhir JSON. Kolom CSV
`pwm` juga ditambahkan di akhir agar parser lama yang membaca 14 kolom pertama
tetap bekerja.

## Autentikasi dashboard

Aksi `/api/override` dan `/api/reset` memerlukan cookie `smartlux_auth`. Token berasal dari `SMARTLUX_TOKEN` di `secrets.h`, atau 12 digit heksadesimal MAC ESP32 yang dicetak ke Serial saat boot bersama URL pendaftaran. Buka `http://smartlux.local/?t=<token>` sekali untuk mendapat cookie `HttpOnly; SameSite=Strict; Path=/`. `/api/status` tetap publik untuk dashboard. Jika `secrets.h` tidak ada, token turunan MAC dapat dihitung oleh siapa pun di LAN — pakai hanya untuk demo pada jaringan lokal yang dipercaya. Token harus 1–64 karakter `[A-Za-z0-9_.-]`; karakter Base64 (`+`, `/`, `=`) ditolak.
