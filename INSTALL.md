# Instalasi (hanya Android: Termux + ZArchiver + GitHub)

Ada dua file bernama `NHC.zip`:
- **NHC.zip (source)**: yang kamu terima di chat ini. Isinya proyek untuk di-push ke GitHub.
- **NHC.zip (release)**: hasil build di dalam artifact `NHC-ARM64-RELEASE`. Isinya `libNHC.so`, dokumen, dan `tools/`.

## 1. Push ke GitHub (Termux)

```bash
pkg install git unzip
unzip NHC.zip
cd NHC
git init
git add .
git commit -m "NHC initial commit"
git branch -M main
git remote add origin https://github.com/DoctorRIDX/NHC.git
git push -u origin main
```

Buat dulu repository kosong bernama `NHC` di GitHub. Jika diminta password, pakai Personal Access Token.

## 2. Build di cloud

1. Buka repository, tab **Actions**, pilih **Build NHC (ARM64)**, tekan **Run workflow**.
2. Tunggu sampai hijau. Jika merah, buka run-nya: ringkasan menampilkan 60 baris terakhir log NDK dan artifact `NHC-BUILD-LOG` berisi log lengkap. Kirim teks error itu ke saya.
3. Di halaman run, bagian **Artifacts**, unduh `NHC-ARM64-RELEASE`.
4. Ekstrak dengan ZArchiver, lalu ekstrak lagi `NHC.zip` di dalamnya. Kamu mendapat `NHC/libNHC.so`.

## 3. Pasang ke game

Syarat: game harus sudah dijalankan lewat **AML 1.4.0** (NHC tidak membawa AML dan tidak mengganti APK game). Ikuti panduan AML untuk itu.

Salin `libNHC.so` ke folder mod AML:

```
/sdcard/Android/data/com.rockstargames.gtasa/mods/
```

Folder alternatif yang disebut dokumentasi AML: `/sdcard/AMLMods/<game>/`. Belum diverifikasi untuk build ini.
Jika ZArchiver tidak bisa membuka `Android/data`, itu pembatasan Android 11+ dan lebih ketat di Android 13+. Itu di luar kendali NHC.

## 4. Jalankan dan baca laporan

Buka game. Tunggu beberapa detik (NHC menunggu Activity sampai 90 detik). Lalu buka `NHC_diagnostics.txt` di folder `files/` data game (jalur pastinya tertulis di menu: SETTINGS, About NHC). Isinya:
- hasil pemeriksaan kompatibilitas,
- fungsi game mana yang ditemukan dan mana yang tidak,
- status tiap fitur dan alasannya.

File `nhc.ini` dibuat otomatis di folder konfigurasi AML (jalurnya juga tertulis di diagnostik).

## 5. Mengisi tabel offset (agar fitur bisa aktif)

NHC tidak menebak alamat. Ambil dari library game yang kamu pakai:

1. Dengan ZArchiver, buka APK game (APK itu ZIP) dan ekstrak `lib/arm64-v8a/libGame.so`.
2. Di Termux:
   ```bash
   pkg install python
   cd /sdcard/Download/NHC/tools      # sesuaikan lokasi
   python nhc_offsets.py exports /sdcard/Download/libGame.so --grep Cheat
   python nhc_offsets.py generate /sdcard/Download/libGame.so --version-name 2.11.311 --version-code <versionCode dari diagnostik>
   ```
3. `generate` mencetak bagian `[Build]` dan `[Offsets_...]`, masing-masing dengan `rva` dan `sig` (16 byte pertama fungsi). Tempel ke `nhc.ini`. Contoh **format** (angkanya bukan data nyata):
   ```
   [Build]
   version_name=2.11.311
   [Offsets_CCheat_JetpackCheat]
   rva=0x<dari tool>
   sig=<32 hex dari tool>
   ```
4. Jika kamu punya RVA fungsi dari sumber yang kamu percaya, tambahkan `--manual CCheat_JetpackCheat=0x...`; tool membaca signature-nya dari file.
5. Jalankan game lagi dan cek `NHC_diagnostics.txt`.

Jika `exports` tidak menampilkan simbol GTA (library di-strip), jalur ini tidak menghasilkan apa-apa dan fitur tetap `NOT IMPLEMENTED`. Itu keterbatasan nyata, bukan bug NHC. Mencari alamat fungsi dari library yang di-strip membutuhkan analisis disassembly yang belum saya lakukan.

## Masalah umum

- **Menu tidak muncul**: lihat diagnostik (Activity tidak ditemukan, Android API di bawah 26, atau `RegisterNatives` gagal). Coba tombol "N" bila swipe bentrok dengan gesture sistem.
- **Semua fitur NOT IMPLEMENTED**: normal bila `nhc.ini` belum berisi entri yang cocok. Alasan per fitur tertulis di menu.
- **Entri "signature mismatch"**: tabel berasal dari build game lain. Buat ulang dari `libGame.so` yang sedang terpasang.
