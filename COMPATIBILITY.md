# Kompatibilitas

Tanggal riset: 2026-11-20.

## Yang diverifikasi dari sumber

| Fakta | Sumber |
|---|---|
| AML mendukung ARM64 (`AML64`, `BYBIT`), header 1.4.0 (`IAML_VER 01040000`), mod di `/sdcard/Android/data/<pkg>/mods`, AML dimuat setelah library game (patch library yang sudah termuat), nama library game `libGame.so` untuk arm64 | `docs/DOCUMENTATION_EN.MD` di repo RusJJ/AndroidModLoader |
| Semua panggilan AML yang dipakai NHC (`GetLib`, `GetLibLength`, `GetLibHandle`, `GetSym`, `Hook`, `GetAppVersionName/Code`, `GetApkMD5`, `GetAndroidVersion`, `GetJavaVM`, `GetCurrentActivity`, `PushToJavaUIThread`, `Config::*`) | dokumentasi yang sama (tanda tangan "exact from iaml.h") |
| GTA SA Android 2.11.311 ada dan dirilis 21 Januari 2026 | catatan rilis Rockstar |
| Build 2.11.311 arm64-v8a memakai `libGame.so` | README proyek gtasa_nx (port Switch) |
| aml-psdk hanya mendukung GTA SA 2.00 dan 2.10 (64-bit) | README aml-psdk |

## Kesimpulan

GTA SA 2.11.311 **tidak** didukung resmi oleh ekosistem AML saat ini. NHC menanganinya dengan sistem pemeriksaan, bukan klaim:

| Pemeriksaan | Dilakukan di |
|---|---|
| ABI arm64 | `compat.cpp` |
| Header AML >= 1.4.0 (error saat compile bila lebih lama) dan dependensi `net.rusjj.aml 1.4.0` | `main.cpp` |
| Android API >= 26 | `compat.cpp` |
| Paket `com.rockstargames.gtasa` | `compat.cpp` + `NEEDGAME` |
| Library game termuat (`libGame.so`, lalu `libGTASA.so`) | `compat.cpp` |
| Tabel offset cocok dengan build (`[Build] version_name`, opsional `version_code`, `apk_md5`) | `compat.cpp` |
| Tiap entri tabel: `rva` dalam library, `sig` 8-32 byte **wajib**, byte di memori sama | `resolver.cpp` |
| Hook benar-benar terpasang | `core.cpp` |

Setiap kegagalan menonaktifkan hanya fitur yang bergantung padanya, dengan alasan tertulis (menu dan `NHC_diagnostics.txt`).

## Belum terbukti

- Build NDK dan pemuatan `libNHC.so` oleh AML di perangkat nyata.
- Apakah `libGame.so` 2.11.311 mengekspor simbol fungsi GTA. Nama simbol di `core.cpp` hanya **kandidat** dan baru dipakai jika benar-benar ada.
- Apakah `aml->Hook` bekerja pada fungsi-fungsi tersebut di ARM64 build ini.
- Apakah `GetCurrentActivity()` mengembalikan referensi yang valid di thread UI.
- Perilaku in-game semua fitur (lihat `FEATURES.md`).
- Android 15/16: pemuatan DEX dari memori dan akses `Android/data` oleh ZArchiver belum diuji.
- Nama ekspor `OnModLoad` dan kawan-kawan mengikuti dokumentasi AML; workflow memeriksanya dengan `llvm-nm` setelah build.

Cakupan Android 15 dan 16: tidak ada kode khusus versi. Yang ditegakkan hanya API >= 26 dan segmen ELF 16 KB.
