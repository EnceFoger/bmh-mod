#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include "config.h"
#include "feature_hooks.h"
#include "hook_util.h"
#include "il2cpp_api.h"
#include "log.h"

// Damage Multiplier + God Mode. Everything is resolved by NAME at runtime (no hard-coded offsets).
//
// Hook 1: Btl.CharacterUnit::SendDamage(int attackerIndex, CharacterUnit target, DamageReaction reaction,
//                                       Btl.Logic.DamageSource damageSource, bool, bool, bool)
//   - "Local team" = Btl.TeamManagerUnit.isLocalTeam of target.teamManager.
//   - target on the local team + god_mode      -> the hit is dropped (original is not called).
//   - target NOT on the local team + multiplier -> DamageSource.coefPermyriad is scaled before the original runs.
//   DamageReaction (64 bytes) and DamageSource (20 bytes) are value types larger than 16 bytes, so on AArch64
//   they travel as pointers to a caller-owned copy; the hook takes them as pointers and edits the copy in place.
//   The trailing MethodInfo* is the 9th argument, passed on the stack.
// Hook 2: Btl.CharacterUnit::IsDead(bool excludeCramped)
//   - god_mode: a local-team unit is never reported dead.
namespace {
using SendDamageFn = void (*)(void*, int32_t, void*, void*, void*, bool, bool, bool, const void*);
using IsDeadFn     = bool (*)(void*, bool, const void*);
SendDamageFn orig_send = nullptr;
IsDeadFn     orig_dead = nullptr;

constexpr size_t kObjHeader = 2 * sizeof(void*);   // value-type field offsets from IL2CPP include the object header
size_t off_team = 0, off_local = 0, off_coef = 0;
bool   g_coef_ok = false;
std::atomic<int> g_logs{0}, g_blocked{0}, g_dead{0};

bool is_local(const void* unit) {
    if (!unit) return false;
    const void* tm = *reinterpret_cast<const void* const*>(static_cast<const char*>(unit) + off_team);
    if (!tm) return false;
    return *(static_cast<const uint8_t*>(tm) + off_local) != 0;
}

void hook_send(void* self, int32_t attacker, void* target, void* reaction, void* src,
               bool last, bool nopopup, bool nofx, const void* mi) {
    const bool  god  = cfg::g.god_mode.load(std::memory_order_relaxed);
    const float mult = cfg::g.damage_multiplier.load(std::memory_order_relaxed);
    if (!god && (mult == 1.0f || !g_coef_ok)) return orig_send(self, attacker, target, reaction, src, last, nopopup, nofx, mi);

    const bool target_local = is_local(target);
    int32_t* coef = (g_coef_ok && src) ? reinterpret_cast<int32_t*>(static_cast<char*>(src) + (off_coef - kObjHeader)) : nullptr;
    if (g_logs.fetch_add(1) < 10)
        LOGI("damage: SendDamage attacker=%d this_local=%d target_local=%d coefPermyriad=%d",
             attacker, (int)is_local(self), (int)target_local, coef ? (int)*coef : -1);

    if (target_local) {
        if (god) {
            if (g_blocked.fetch_add(1) < 5) LOGI("god_mode: hit on local team dropped");
            return;
        }
    } else if (mult != 1.0f && coef && *coef > 0) {
        double v = std::llround(static_cast<double>(*coef) * mult);
        *coef = static_cast<int32_t>(std::min<double>(v, 2147483647.0));
    }
    orig_send(self, attacker, target, reaction, src, last, nopopup, nofx, mi);
}

bool hook_dead(void* self, bool exclude_cramped, const void* mi) {
    bool r = orig_dead(self, exclude_cramped, mi);
    if (r && cfg::g.god_mode.load(std::memory_order_relaxed) && is_local(self)) {
        if (g_dead.fetch_add(1) < 5) LOGI("god_mode: IsDead overridden to false for a local-team unit");
        return false;
    }
    return r;
}
}  // namespace

bool features::install_damage(bool dry_run) {
    using namespace il2cpp;
    void* cu = find_class("Btl", "CharacterUnit");
    void* tm = find_class("Btl", "TeamManagerUnit");
    void* ds = find_class("Btl.Logic", "DamageSource");
    if (!cu || !tm) { LOGW("damage: CharacterUnit/TeamManagerUnit not found - damage_multiplier and god_mode unavailable"); return false; }

    const bool have_team = instance_field(cu, "CharacterUnit", "<teamManager>k__BackingField", "Btl.TeamManagerUnit", &off_team) &&
                           instance_field(tm, "TeamManagerUnit", "<isLocalTeam>k__BackingField", "System.Boolean", &off_local);

    // coefPermyriad lives in a value type; it must be passed by pointer (> 16 bytes unboxed) for the hook to edit it.
    g_coef_ok = false;
    if (ds && api.class_is_valuetype && api.class_instance_size && api.class_is_valuetype(ds) &&
        api.class_instance_size(ds) - (int32_t)kObjHeader > 16 &&
        instance_field(ds, "DamageSource", "coefPermyriad", "System.Int32", &off_coef) && off_coef >= kObjHeader)
        g_coef_ok = true;
    else
        LOGW("damage: DamageSource.coefPermyriad not usable - damage_multiplier disabled");

    MethodRef send, dead;
    const bool have_send = resolve("Btl", "CharacterUnit", "SendDamage", 7, "System.Void",
                                   {"System.Int32", "Btl.CharacterUnit", "DamageReaction", "Btl.Logic.DamageSource",
                                    "System.Boolean", "System.Boolean", "System.Boolean"}, send);
    const bool have_dead = resolve("Btl", "CharacterUnit", "IsDead", 1, "System.Boolean", {"System.Boolean"}, dead);
    if (!have_team || !have_send) { LOGW("damage: required fields/method missing - nothing hooked"); return false; }
    if (dry_run) { LOGI("probe: damage targets OK (not hooked) coef=%d isdead=%d", (int)g_coef_ok, (int)have_dead); return true; }

    if (!install_hook("damage(SendDamage)", send.addr, reinterpret_cast<void*>(&hook_send),
                      reinterpret_cast<void**>(&orig_send), true)) return false;
    if (!have_dead || !install_hook("god_mode(IsDead)", dead.addr, reinterpret_cast<void*>(&hook_dead),
                                    reinterpret_cast<void**>(&orig_dead), true))
        LOGW("god_mode: IsDead hook not installed - god_mode only drops incoming hits");
    return true;
}
