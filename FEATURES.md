# Daftar fitur dan status

"Kode + tes PC" berarti logika dijalankan dengan fungsi game palsu di PC. **Tidak ada fitur yang sudah diuji di dalam game.**

| Fitur | Mekanisme | Butuh (harus terverifikasi) | Kode + tes PC | Di game |
|---|---|---|---|---|
| Infinite Money | Memanggil `CCheat::MoneyArmourHealthCheat()` tiap 3 detik di game thread (+$250,000, sekaligus isi health dan armor) | `FindPlayerPed`, `CGame::Process`, cheat uang | ya | belum diuji |
| Add $250,000 once | Panggilan yang sama, sekali | sama | ya | belum diuji |
| Invincibility | Hook `CPed::InflictDamage`; damage ke ped pemain dibatalkan. Jalur damage lain (jatuh, api, tenggelam) tidak dijamin tercakup | `FindPlayerPed`, `CPed::InflictDamage` | ya | belum diuji |
| Jetpack: Activate | `CCheat::JetpackCheat()` | `FindPlayerPed`, `CGame::Process`, jetpack cheat | ya | belum diuji |
| Jetpack: Deactivate | **NOT IMPLEMENTED**: tidak ada fungsi game terverifikasi untuk melepas jetpack | - | tidak ada | - |
| Vehicle Spawn | `CCheat::VehicleCheat(model)`; posisi spawn dipilih fungsi bawaan game di dekat pemain. Model dibatasi pada daftar kendaraan yang bisa dikendarai (122 item) | `FindPlayerPed`, `CGame::Process`, vehicle cheat | ya | belum diuji |
| Vehicle List | Daftar pilih di menu (katalog id 400-611) | - | ya | n/a |
| Weapon Selector | `CPed::GiveWeapon(ped, id, ammo, flag)`; 39 senjata, amunisi 1-9999 (langkah -/+) | `FindPlayerPed`, `CGame::Process`, `GiveWeapon` | ya | belum diuji |
| Menu sentuh (swipe/tombol, scroll, portrait/landscape, submenu, animasi) | `NHCMenu.java` dimuat dari DEX | Android API 26+, Activity | kompilasi tipe | belum diuji |
| Settings (visibility, UI scale, theme, about) | `nhc.ini` | - | ya | belum diuji |

Fitur yang tidak memenuhi syarat menampilkan `NOT IMPLEMENTED` beserta fungsi yang kurang. Status default setelah instalasi pertama: semua `NOT IMPLEMENTED`, sampai `nhc.ini` diisi dengan entri terverifikasi (lihat `INSTALL.md`).
