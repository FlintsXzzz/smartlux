## Ringkasan

<!-- Apa yang berubah dan mengapa. -->

## Jenis perubahan

- [ ] Firmware / dashboard
- [ ] Dokumen / proposal
- [ ] Enclosure
- [ ] Perkakas / CI

## Verifikasi

- [ ] `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX` lulus
- [ ] `pio run -d firmware/SMARTLUX` lulus
- [ ] Tidak ada kredensial di diff (`secrets.h` tetap diabaikan)

## Catatan

<!-- Batasan, uji yang belum bisa dijalankan, dsb. -->
