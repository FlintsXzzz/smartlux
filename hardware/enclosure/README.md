# Casing SMARTLUX (OpenSCAD)

`smartlux_case.scad` adalah model parametris untuk casing prototipe SMARTLUX. Model ini hanya memakai primitif OpenSCAD standar (tanpa BOSL2/MCAD), sehingga dapat langsung dirender dengan OpenSCAD 2019.05 atau lebih baru.

## Isi berkas

- `smartlux_case.scad` — satu berkas untuk dua bagian:
  - `PART = "case"` — badan casing (dasar, empat boss sudut M3, dudukan ESP32/INA219, lubang USB/DC jack/ventilasi, tab pengikat kabel, dan tulisan timbul "SMARTLUX").
  - `PART = "lid"` — penutup (jendela OLED 26 × 14 mm, lubang tombol 12 mm, kubah PIR 24 mm, jendela LDR 5 mm, empat lubang sekrup M3 yang di-counterbore, dudukan OLED/INA219/PIR).
  - `PART = "both"` — pratinjau terpisah (penutup diangkat 12 mm).

## Dimensi yang diasumsikan

Nilai berikut adalah asumsi untuk modul yang umum dijual. Setiap nilai yang diberi tanda `MEASURE` di dalam `.scad` **wajib diukur dengan jangka sorong** pada modul yang benar-benar dipakai sebelum mencetak.

| Bagian | Nilai | Keterangan |
| ------ | ----- | ---------- |
| Rongga dalam | 110 × 75 × 40 mm | X × Y × Z di atas dasar |
| Dinding / dasar / penutup | 2,4 mm | ruler ketebalan cetakan |
| Casing luar | 114,8 × 79,8 × 44,8 mm | termasuk penutup |
| ESP32 DevKit | 55 × 28 mm, 4 × M3 | jarak lubang 48,5 × 22,9 mm (`MEASURE`) |
| OLED SSD1306 0,96" | 27 × 27 mm, 4 × M2 | jendela terlihat 26 × 14 mm |
| INA219 | 26 × 21 mm, 2 × M2 | dipasang di dasar casing |
| PIR HC-SR501 | 32 × 24 mm, 4 × M2,5 | bukaan kubah 24 mm |
| Tombol panel | 12 mm | lubang di penutup |
| Jendela LDR | 5 mm + reses 8 mm | reses berfungsi sebagai light-pipe |
| USB / DC jack | 12 × 8 mm / 9 × 11 mm | dinding kiri / kanan |
| Ventilasi | 3 slot 40 × 3 mm | dinding belakang |
| Toleransi umum | 0,3 mm | variable `tol` di `.scad` |

## Cara mengekspor STL

Pasang OpenSCAD, lalu jalankan dari folder ini:

```bash
openscad -o smartlux_case.stl -D 'PART="case"' smartlux_case.scad
openscad -o smartlux_lid.stl  -D 'PART="lid"'  smartlux_case.scad
```

Berkas `.stl` sengaja **tidak disimpan** di repository (pola `*.stl` ada di `.gitignore`). Hasil cetak bergantung pada printer dan harus dihasilkan ulang setelah mengukur komponen.

Jika font tidak tersedia, ubah `label_on = false` pada bagian `[Optional embossed label]`.

## Pengaturan cetak yang disarankan

- Bahan: PETG (lebih tahan panas dan lembap daripada PLA di dekat regulator ESP32).
- Ketebalan lapis: 0,2 mm; nozzle 0,4 mm.
- Dinding: 3 perimeter; infill 20% (grid/gyroid).
- Support: **mati** (dudukan dan boss dirancang bisa dicetak langsung). Brim opsional untuk badan casing jika adhesi tempat tidur kurang.
- Penutup dicetak dengan **permukaan luar menghadap ke bawah** (lihat komentar di bagian LID pada `.scad`) agar tidak perlu support.
- Setelah dicetak, uji pasang kering (dry-fit) sebelum memasang sekrup. Jika lubang terlalu ketat, naikkan `tol` 0,1–0,2 mm dan cetak ulang.

## Perakitan

1. Cetak badan dan penutup. Bersihkan sisa filament pada lubang sekrup dan jendela.
2. Pasang ESP32 di dasar casing memakai 4 sekrup M3 pada dudukan (tinggi 5 mm). Jangan menjauhkan lubang USB dari lubang 12 × 8 mm di dinding kiri.
3. Pasang INA219 di dasar casing memakai 2 sekrup M2.
4. Pasang OLED di bawah penutup memakai 4 sekrup M2, lalu PIR memakai 4 sekrup M2,5. Pastikan jendela OLED dan kubah PIR keluar dari lubang yang sesuai.
5. Pasang tombol override 12 mm pada lubangnya dan LDR pada jendela 5 mm. Gunakan pipa light-pipe bila jendela terlalu dalam.
6. Rangkai kabel sesuai `firmware/SMARTLUX/README.md`: I²C SDA GPIO21 / SCL GPIO22, tombol GPIO32 (`INPUT_PULLUP`, kaki lain ke GND), PIR GPIO27, MOSFET GPIO26, LDR GPIO34. Semua ground disatukan.
7. Masukkan penutup; dua rib pada penutup akan masuk ke rongga sebagai panduan. Kunci dengan 4 sekrup M3 counterbore pada boss sudut.
8. Nyalakan sistem dan periksa: tombol tidak macet, OLED terbaca, PIR masih mendeteksi gerakan melalui kubah, tidak ada kabel yang terjepit, dan ventilasi tidak tertutup.

## Catatan keselamatan dan keterbatasan

- Casing ini hanya untuk prototipe meja dengan sumber DC 5 V. Jangan memasang jaringan PLN 220 V di dalam casing.
- Soket DC dan USB tidak boleh diberi dua sumber daya bersamaan; lepaskan kabel USB pemrograman setelah firmware terpasang bila prototipe memakai adaptor 5 V sendiri.
- PIR dan OLED berada langsung di bawah penutup; lakukan uji suhu singkat setelah 30 menit operasi. Jika penutup terasa panas, perbesar ventilasi atau gunakan PETG dengan warna terang.
- Toleransi cetak sangat bergantung pada printer, suhu, dan kalibrasi. Ukur ulang setelah cetak dan sesuaikan variabel di `.scad` bila perlu.
- Model ini belum dirender di mesin yang dipakai menyusun laporan (OpenSCAD tidak terpasang), sehingga belum ada verifikasi STL aktual. Render sendiri dengan perintah di atas sebelum dicetak.
