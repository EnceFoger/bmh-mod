# Kuro Mirror High (Zygisk)

Modul Zygisk untuk Bleach Mirrors High (`com.bandainamcoent.bleach_mh_ww`). Target dicari lewat NAMA saat runtime
(tanpa offset), dikendalikan file teks, dengan log dan `stage` untuk diagnosa.

## Fitur
| Fitur | Target | Status |
|---|---|---|
| Unity Speed (`unity_speed`) | `UnityEngine.Time.set_timeScale` (+ `get_deltaTime`) | nama ada di metadata, belum dites di device |
| Game Speed (`game_speed`) | `get_BattleSpeedRate()` milik game (class dicari otomatis, harus return `System.Single`) | nama ada di metadata, efek belum terbukti |

Damage rate tidak ada di modul ini: target method-nya belum bisa dipastikan. `damage_dump=1` hanya menulis daftar
kandidat ke log (tidak mengubah gameplay) supaya target yang benar bisa ditentukan.

## Pakai
1. Upload isi zip ke repo GitHub (flat). Pindahkan `build.yml.txt` ke `.github/workflows/build.yml`.
2. Jalankan workflow (Actions -> Run workflow), unduh artifact `kuro_mirror_high.zip`.
3. Flash di Magisk (Zygisk aktif), buka game sekali. Config dibuat di
   `/data/data/com.bandainamcoent.bleach_mh_ww/files/kuro_mirror_high.cfg`
   (atau buat sendiri di `/sdcard/Android/data/com.bandainamcoent.bleach_mh_ww/files/`).
4. Edit config (`stage=3`, `unity_speed=2.0`, `game_speed=2.0`), berlaku ~2 dtk. Lihat log: `adb logcat -s KuroMirrorHigh`.

Belum pernah dikompilasi/dijalankan di device (tidak ada NDK di sandbox).
