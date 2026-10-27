#pragma once
namespace features {
bool install_unity_speed(bool dry_run);   // UnityEngine.Time.set_timeScale (+ get_deltaTime re-apply)
bool install_game_speed(bool dry_run);    // game's own get_BattleSpeedRate (found by name at runtime)
bool install_damage_rate(bool dry_run);   // discovery: logs damage-related methods; no hook yet
}
