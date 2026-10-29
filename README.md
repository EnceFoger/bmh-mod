# Kuro Mirror High (Zygisk)

Modul Zygisk untuk Bleach Mirrors High (`com.bandainamcoent.bleach_mh_ww`). Target dicari lewat NAMA saat runtime
(tanpa offset), dikendalikan file teks, dengan log dan `stage` untuk diagnosa.

## Fitur
- **Unity Speed** (`unity_speed`): mengalikan `UnityEngine.Time.timeScale`. Sudah terbukti jalan di device.
  Batas default 5.0, naikkan dengan `max_unity_speed` (maks 100).
- **Damage Rate** (`damage_rate`): damage yang dikeluarkan tim kamu dikali nilai ini (batas `max_damage_rate`, default 10).
- **God Mode** (`god_mode=1`): tim kamu tidak menerima damage.

Damage Rate dan God Mode memakai satu hook di `Btl.CharacterUnit::SendDamage`, yang mengubah
`DamageSource.coefPermyriad` (10000 = 100%) sebelum method asli jalan. Belum terbukti di device:
cek log, baris `battle: SendDamage attackerLocal=.. targetLocal=.. coef A -> B`.

## Pakai
1. Upload isi zip ke repo GitHub (flat), jalankan workflow, flash artifact `kuro_mirror_high.zip`, reboot.
2. Buka game sekali: `kuro_mirror_high.cfg` dibuat otomatis (stage=3) di
   `/sdcard/Android/data/com.bandainamcoent.bleach_mh_ww/files/`. Edit `unity_speed`, `damage_rate`, `god_mode`,
   lalu restart game. Log: `kuro_mirror_high.log` di folder yang sama.
