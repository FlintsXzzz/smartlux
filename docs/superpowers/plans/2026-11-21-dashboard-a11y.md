# Rencana: perbaikan aksesibilitas dashboard (2026-11-21)

## Tujuan
Memperbaiki temuan audit `code-reviewer` pada blok `INDEX_HTML` di
`firmware/SMARTLUX/SMARTLUX.ino` (baris 253–412) tanpa mengubah tampilan,
endpoint, kunci JSON, pin, atau timing.

## Batasan
- Salinan bahasa Indonesia, tema gelap, lebar 360 px sampai 1440 px.
- Semua ID elemen, empat endpoint, 23 kunci JSON, polling 1 dtk, CSP
  `unsafe-inline`, dan helper retry 429 tidak berubah.
- Anggaran halaman 8.192 byte (8 KB PROGMEM). Halaman awal: 8.122 byte.

## Keputusan
| Temuan | Tindakan | Delta byte |
|---|---|---:|
| Stale opacity merusak kontras saat terputus | Hapus aturan `.stale` | −60 |
| JS `stale` jadi mati setelah aturan dihapus | Hapus dua `classList` `.stale` | −77 |
| Status lampu tidak diumumkan pembaca layar | `role="status"` + penjaga perubahan teks | +51 |
| Target sentuh `.reset` ~37 px | `min-height:44px` | +16 |
| Fokus `.reset` tanpa outline kustom | Perluas `:focus-visible` | +21 |
| Putusnya koneksi tidak diumumkan | `role="status"` + penjaga perubahan IP | +43 |
| Tidak ada `<h1>` | Ubah brand ke `<h1>` + reset gaya UA | +25 |
| Teks "Mati" 3,29:1 | Ganti `#5b6572` → `#707b8a` | 0 |

Total bersih +19 byte → 8.141 byte, menyisakan 51 byte.

## Ditolak
- `aria-pressed` pada tombol mode (+112 B): tidak muat tanpa menghapus jalur
  `demo`/`mock()` yang dipakai screenshot gan-harness. Catat sebagai kerja lanjutan.
- `min-height` pada `.why`: perubahan tata letak, bukan perbaikan mekanis.
- Menghapus `:hover` pada `.reset`: tidak lagi perlu setelah ada ruang.

## Validasi (hasil)
- `arduino-cli compile --fqbn esp32:esp32:esp32 firmware/SMARTLUX` PASS:
  1.027.000 B flash (78%), 51.360 B RAM (15%).
- Halaman 8.143 byte, 49 byte di bawah anggaran 8.192 (prediksi 8.141;
  selisih 2 byte dari panjang string penjaga).
- Empat `server.on()` dan ke-22 ID elemen sama persis dengan iterasi 03;
  kunci JSON tidak disentuh karena blok C++ di luar `INDEX_HTML` tidak diubah.
- Kontras teks "Mati" naik dari 3,29:1 menjadi 4,53:1; referensi `.stale`
  sudah nol di dalam halaman.
- Tidak ada uji LAN/kontras di perangkat: tidak ada hardware yang tersambung.
  `aria-pressed` belum dipasang (butuh ±112 B, tidak muat).
