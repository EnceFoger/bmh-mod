# Kuro Mirror High (Zygisk)

Modul Zygisk untuk Bleach Mirrors High (`com.bandainamcoent.bleach_mh_ww`). Target dicari lewat NAMA saat runtime
(tanpa offset), dikendalikan file teks, dengan log dan `stage` untuk diagnosa.

## Fitur
- **Unity Speed** (`unity_speed`): mengalikan `UnityEngine.Time.timeScale`. Terbukti ter-hook di device (lihat log).
  Batas default 5.0, naikkan dengan `max_unity_speed=10` (maks 100).

- **Damage Multiplier** (`damage_multiplier`, 1.0 = off, 0.1 - 100): mengalikan `DamageSource.coefPermyriad` pada hit yang
  mengenai sisi musuh (target bukan `isLocalTeam`). Hook: `Btl.CharacterUnit::SendDamage`.
- **God Mode** (`god_mode=1`): hit yang mengenai tim lokal dibuang, dan `Btl.CharacterUnit::IsDead` tidak pernah true untuk tim lokal.

Catatan: rumus damage asli game ter-obfuscate dan belum terlihat di dump. Efek `damage_multiplier` bergantung pada
apakah game memakai `coefPermyriad` saat menghitung damage. Log `damage:` / `god_mode:` mencatat tiap hit pertama untuk verifikasi.

Game Speed belum ada: `get_BattleSpeedRate` di game ini ternyata setting tersimpan bertipe Int32
(`Dimps.Alma.GameData.LocalData`), bukan pengali kecepatan, dan target damage belum bisa dipastikan.
`dump=1` hanya menulis daftar method terkait ke log (tidak mengubah gameplay) untuk mencari target yang benar.

## Pakai
1. Upload isi zip ke repo GitHub (flat). Pindahkan `build.yml.txt` ke `.github/workflows/build.yml`.
2. Jalankan workflow (Actions -> Run workflow), unduh artifact `kuro_mirror_high.zip`, flash (Zygisk aktif).
3. Buat `/sdcard/Android/data/com.bandainamcoent.bleach_mh_ww/files/kuro_mirror_high.cfg`:
   `stage=3` dan `unity_speed=2.0`. Restart game. Log: `kuro_mirror_high.log` di folder yang sama.
