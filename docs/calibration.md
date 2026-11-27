# Kalibrasi dan Validasi Pengukuran

Panduan singkat untuk menyiapkan SMARTLUX sebelum pengambilan data praktikum.
Rincian metodologi ada di proposal (BAB III); dokumen ini hanya urutan kerja.

## 1. Sumber daya tunggal

- Gunakan satu sumber 5 V teregulasi. Cabut kabel USB pemrograman saat mengukur
  agar tidak ada dua sumber pada board.
- Jangan melebihi 12 V dan jangan menyambungkan ke listrik PLN.

## 2. Pembagi LDR (GPIO34)

1. Letakkan sensor pada kondisi terang acuan dan catat ADC mentah dari Serial
   (keluaran CSV) atau dari `light_adc` pada `/api/status`.
2. Ulangi pada kondisi gelap acuan dan catat ADC.
3. Tetapkan ambang di antara kedua nilai dengan histeresis 10–20 %
   (mis. gelap 1800, terang 2100), lalu sesuaikan nilai ambang pada bagian
   konfigurasi firmware. Jangan pernah memberi 5 V ke GPIO34; batasnya 3,3 V.
4. Firmware membaca ADC lima kali dan memakai nilai tengah (median) sebelum
   memutuskan status cahaya. Catat nilai `light_adc` yang sudah stabil, bukan
   satu pembacaan sesaat yang bisa melonjak.

## 3. INA219 (0x40, shunt 0,1 Ω)

1. Pasang INA219 secara high-side: sumber → VIN+ → VIN- → rel 5 V.
2. Bandingkan `voltage_v` dan `current_ma` pada `/api/status` dengan multimeter
   (V dan A) pada beban yang sama.
3. Kalibrasi memakai `setCalibration_32V_2A()` (rentang ±3,2 A). Catat simpangan
   pada tabel pengamatan.

## 4. BH1750 opsional (0x23, I²C)

1. Sambungkan BH1750 ke SDA/SCL 3,3 V yang sama dengan OLED/INA219 (alamat 0x23,
   tidak bentrok). Sensor terdeteksi otomatis saat boot (`# BH1750 siap`).
2. Bandingkan `lux_bh` di `/api/status` dengan luxmeter atau aplikasi ponsel pada
   posisi yang sama. Akurasi khas ±20%; catat simpangannya, jangan klaim sebagai
   alat ukur terkalibrasi.
3. Sesuaikan `LUX_DARK_ON` (50 lx) dan `LUX_BRIGHT_OFF` (80 lx) bila kondisi
   ruangan uji berbeda; jaga nilai gelap < terang.
4. Tanpa BH1750, firmware otomatis memakai ambang ADC LDR seperti biasa; tidak
   ada perubahan kabel yang wajib.

## 5. PIR HC-SR501 (GPIO27, OUT)

- Ukur tegangan OUT dengan multimeter; OUT harus ≤ 3,6 V sebelum disambungkan
  ke GPIO27.
- Beri waktu pemanasan 60 detik dan pilih mode H (repeat) sesuai rencana.

## 6. Pencatatan

- Energi diintegrasikan oleh firmware (`E_Wh += V·I·dt/3600`) tiap detik.
  Tekan **Reset energi** di dashboard (`/api/reset`) di awal setiap percobaan.
- Simpan keluaran CSV Serial (header ditambah lima kolom status) untuk analisis
  mean ± SD dan CI 95 % sesuai proposal.

## Verifikasi akhir

- [ ] Ambang LDR terisi dan stabil pada kondisi terang/gelap
- [ ] (Bila dipasang) `lux_bh` cocok dengan luxmeter dalam ±20%
- [ ] INA219 cocok dengan multimeter (V dan A)
- [ ] Delay 5–200 dtk dan PWM tersimpan setelah reboot
- [ ] Tegangan OUT PIR ≤ 3,6 V terukur
- [ ] Satu sumber 5 V, USB dicabut saat mengukur
