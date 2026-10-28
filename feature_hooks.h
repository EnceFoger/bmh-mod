#pragma once
namespace features {
bool install_unity_speed(bool dry_run);   // UnityEngine.Time.set_timeScale (+ get_deltaTime re-apply)
bool install_game_speed(bool dry_run);    // game's own get_BattleSpeedRate (found by name at runtime)
void dump_damage_candidates();            // diagnostic log only (damage_dump=1), never hooks
}
