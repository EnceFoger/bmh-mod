#pragma once
namespace features {
bool install_unity_speed(bool dry_run);   // UnityEngine.Time.set_timeScale (+ get_deltaTime re-apply)
bool install_damage(bool dry_run);        // damage_multiplier + god_mode (Btl.CharacterUnit::SendDamage / IsDead)
void dump_candidates();                   // diagnostic log only (dump=1), never hooks
}
