#pragma once
namespace features {
bool install_unity_speed(bool dry_run);   // UnityEngine.Time.set_timeScale (+ get_deltaTime re-apply)
bool install_battle(bool dry_run);        // Btl.CharacterUnit::SendDamage: god mode + damage multiplier
void dump_candidates();                   // diagnostic log only (dump=1), never hooks
}
