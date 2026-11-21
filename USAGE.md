# Memakai menu NHC

## Membuka dan menutup

- **Swipe turun** dari tepi paling atas layar (strip tak terlihat setinggi 18 dp). Pada mode immersive, gesture sistem Android bisa memakai swipe pertama; ulangi swipe atau pakai tombol.
- **Tombol "N"** melayang: ketuk untuk buka/tutup, seret untuk memindahkan.
- Tutup dengan tombol CLOSE, ketuk di luar panel, atau ketuk "N" lagi.
- Panel di-scroll dengan geser jari; portrait memakai panel lebar di tengah atas, landscape memakai panel di sisi kanan.

Strip swipe mengambil sentuhan di 18 dp teratas layar. Bila itu mengganggu game, pilih SETTINGS, Menu Visibility, "Button only".

## Struktur menu

```
NHC MOD MENU
├── PLAYER    Infinite Money, Add $250,000 once, Invincibility, Jetpack (Activate; Deactivate = NOT IMPLEMENTED)
├── VEHICLES  Vehicle Spawn (kendaraan terpilih), Vehicle List (daftar pilih)
├── WEAPONS   Weapon Selector (daftar), Ammo -/+, Give
└── SETTINGS  Menu Visibility, UI Scale 60-160%, Theme (3 warna merah), About NHC
```

Baris fitur menampilkan `ON/OFF`. Jika fungsi game belum terverifikasi, baris menampilkan `NOT IMPLEMENTED - <alasan>` dan tidak memanggil apa pun. Baris paling bawah panel menampilkan hasil aksi terakhir (misalnya "Queued", "no player in the world").

Semua aksi dikirim ke game thread dan dijalankan pada frame berikutnya, bukan dari thread UI.

Pengaturan (`UiScale`, `Theme`, `Visibility`) disimpan di `nhc.ini`.
