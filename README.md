# NHC — Napoleon Hororss-Center

Plugin **Android Mod Loader (AML)** untuk GTA San Andreas Android (target 2.11.311, ARM64-v8a) dengan menu sentuh gelap-merah.
Dibuild sepenuhnya di GitHub Actions, jadi kamu hanya butuh HP (ZArchiver + Termux).

## Status jujur (baca ini dulu)

| Hal | Status |
|---|---|
| Logika inti (resolver, cek kompatibilitas, antrean fitur, katalog) | Ditulis dan **dites unit di PC** (602 pengecekan, termasuk uji mutasi) |
| Kode C++ Android (`platform.cpp`, `main.cpp`, `menu.cpp`) | Hanya **dicek sintaks** terhadap stub API AML/JNI. **Belum di-build NDK** |
| Menu Java | Lolos kompilasi tipe terhadap stub Android. **Belum jadi DEX, belum dijalankan** |
| Build `libNHC.so` | **Belum dijalankan.** Dikerjakan workflow `build.yml` di GitHub |
| Dimuat AML di game | **Belum diuji** |
| Fitur di dalam game | **Belum diuji** |
| Offset untuk GTA SA 2.11.311 | **Tidak ada yang terverifikasi.** Tabel resmi AML (aml-psdk) hanya mendukung 2.00 dan 2.10 64-bit |

Akibatnya, **setelah instalasi pertama semua fitur tampil `NOT IMPLEMENTED`** dengan alasan yang jelas, bukan tombol palsu.
Fitur aktif hanya jika fungsi game yang dibutuhkan terbukti ada: diekspor dengan nama oleh library game, atau tercatat di `nhc.ini` **dengan signature byte yang cocok** dengan library yang sedang berjalan.
Cara mendapatkannya dari HP ada di `INSTALL.md` (bagian "Mengisi tabel offset").

## Cara kerja singkat

1. `OnModLoad` menjalankan pemeriksaan kompatibilitas: ABI, header AML 1.4.0, Android API 26+, paket game, versi game, library game (`libGame.so` atau `libGTASA.so`).
2. Tiap fungsi game dicari lewat tabel `nhc.ini` (wajib `rva` + `sig`) atau simbol yang diekspor. Byte di memori dibaca aman (tanpa risiko crash) dan harus sama dengan `sig`.
3. Hook dipasang hanya pada fungsi yang lolos verifikasi: `CGame::Process` (agar aksi berjalan di game thread) dan `CPed::InflictDamage` (invincibility).
4. Menu Java ditanam sebagai DEX di dalam `libNHC.so`, dimuat dengan `InMemoryDexClassLoader`, dan dibuka dengan swipe turun dari tepi atas atau tombol "N" yang bisa digeser.
5. `NHC_diagnostics.txt` ditulis ke folder data game: laporan lengkap apa yang ditemukan dan kenapa suatu fitur mati.

## Struktur proyek

```
NHC/
├── .github/workflows/build.yml   build ARM64 + DEX + paket NHC.zip
├── Android.mk, Application.mk    ndk-build (arm64-v8a, C++17, c++_static)
├── include/nhc/                  header internal
├── include/mod/                  diisi workflow dari repo AML (header resmi)
├── src/
│   ├── main.cpp                  deklarasi mod AML dan callback
│   ├── platform.cpp              satu-satunya pembungkus API AML
│   ├── compat.cpp                pemeriksaan kompatibilitas
│   ├── resolver.cpp              tabel offset + signature, simbol terekspor
│   ├── core.cpp                  hook, antrean game-thread, ketersediaan fitur, diagnostik
│   ├── player.cpp, vehicles.cpp, weapons.cpp   fitur + katalog
│   ├── settings.cpp              UiScale, Theme, Visibility
│   ├── menu.cpp                  jembatan JNI ke menu Java
│   └── safe_read.cpp, ui_dex.cpp
├── java/com/nhc/menu/NHCMenu.java   menu sentuh
├── tools/nhc_offsets.py          buat entri offset dari libGame.so (Termux)
├── tools/build_ui_dex.sh, embed_dex.py
├── tests/                        tes unit di PC (make -C tests)
├── mod/nhc.ini.example           pengaturan bawaan
└── README.md INSTALL.md USAGE.md COMPATIBILITY.md FEATURES.md LICENSE
```

## Versi yang dipakai

- AML mod headers: 1.4.0 (`IAML_VER 01040000`), diambil dari `RusJJ/AndroidModLoader` (ref default `main`; commit-nya dicetak di log Actions, sebaiknya di-pin setelah build pertama sukses)
- NDK r27c, JDK 17 (Temurin), C++17, `APP_PLATFORM android-21`, ARM64 saja, segmen ELF 16 KB
- Menu butuh Android API 26+ (`InMemoryDexClassLoader`)
- Action: `actions/checkout@v4`, `actions/setup-java@v4`, `actions/upload-artifact@v4`, `nttld/setup-ndk@v1` (versi terbaru saat ini tidak sempat diperiksa, jadi pakai major yang saya kenal)

## Lisensi

MIT. NHC tidak memakai kode aml-psdk (GPL-3.0); header AML (MIT) hanya dipakai saat build.
