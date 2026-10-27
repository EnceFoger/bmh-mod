# Speed Companion (Zygisk)

Konsep sama seperti Kuro Companion: modul Zygisk terpisah, target dicari lewat NAMA saat runtime (tanpa offset),
dikendalikan file teks, log + `stage` untuk diagnosa. Dianalisis dari `libil2cpp.so` + `global-metadata.dat` (v39) yang kamu kirim.

## Penting: build ini BUKAN build yang dipakai Kuro
Di metadata ini tidak ada `Bleach.Games.*`, `CalculateDamageUpRate`, `GameTimeManager` dll. (target Kuro Companion lama
pasti "NOT FOUND"). Namespace game di sini beda (mis. `ALMA.*`), jadi fitur lama tidak dipakai ulang.

## Fitur
| Fitur | Target | Status |
|---|---|---|
| Unity Speed | `UnityEngine.Time.set_timeScale` (+ `get_deltaTime`) | nama ada di metadata, mekanisme sama dengan Kuro |
| Game Speed | `get_BattleSpeedRate()` milik game (class dicari otomatis, harus return `System.Single`) | nama ada di metadata, efek belum terbukti |
| Damage Rate Up | kandidat: `SendDamage`, `OnReceivedDamage`, `RegisterDamage`, `SetDamageValue`, ... | **hanya discovery**: dicatat ke log, TIDAK di-hook |

Kenapa damage belum di-hook: ada banyak method "damage" dan tidak bisa diverifikasi mana yang membawa angka final
tanpa menjalankan game. Hook asal bisa crash atau tidak berefek. Jalankan sekali (stage=2 atau 3), kirim isi
`speed_companion.log` (baris `damage candidate:`), lalu target satu method itu bisa dipasang dengan benar.

## Pakai
1. Upload isi zip ke repo GitHub. Semua file flat; `build.yml.txt` dipindah ke `.github/workflows/build.yml`.
2. Jalankan workflow (Actions -> Run workflow), unduh artifact `speed_companion.zip`.
3. Isi `package.txt` (di zip, atau setelah flash di `/data/adb/modules/speed_companion/package.txt`) dengan package game.
4. Flash di Magisk (Zygisk aktif), buka game sekali. Config dibuat di
   `/data/data/<package>/files/speed_companion.cfg` (bisa juga `/sdcard/Android/data/<package>/files/`).
5. Edit config (`stage=3`, `unity_speed=2.0`, ...), berlaku ~2 dtk. Log: `speed_companion.log` / `adb logcat -s SpeedCompanion`.
