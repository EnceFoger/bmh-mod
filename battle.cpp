#include <algorithm>
#include <atomic>
#include <cstdint>
#include "config.h"
#include "feature_hooks.h"
#include "hook_util.h"
#include "il2cpp_api.h"
#include "log.h"

// God mode + damage multiplier through ONE hook on Btl.CharacterUnit::SendDamage.
// Its by-value struct argument DamageSource (20 bytes unboxed -> passed by pointer on ARM64) holds
// coefPermyriad (10000 = 100 %). The coefficient is edited in the caller's copy before the original runs:
//   target is on the local team (and god_mode)        -> 0
//   local team hits the opponent (damage_rate != 1)   -> coef * damage_rate
// All offsets come from IL2CPP field lookups by name. The value is range-checked; on a layout mismatch the
// hook stops editing and logs why.
namespace {
using SendFn = void (*)(void*, int32_t, void*, void*, void*, bool, bool, bool, const void*);
SendFn orig_send = nullptr;
size_t off_tm = 0, off_local = 0, off_coef = 0;
std::atomic<int>  g_logs{0};
std::atomic<bool> g_bad{false};

bool is_local(void* unit) {
    if (!unit) return false;
    void* tm = *reinterpret_cast<void**>(reinterpret_cast<char*>(unit) + off_tm);
    if (!tm) return false;
    return *reinterpret_cast<bool*>(reinterpret_cast<char*>(tm) + off_local);
}

void hook_send(void* self, int32_t atk, void* target, void* reaction, void* dsrc, bool last, bool popup, bool fx, const void* mi) {
    if (dsrc && !g_bad.load(std::memory_order_relaxed)) {
        int32_t* coef = reinterpret_cast<int32_t*>(reinterpret_cast<char*>(dsrc) + off_coef);
        int32_t c = *coef;
        if (c < 0 || c > 100000000) {
            if (!g_bad.exchange(true)) LOGW("battle: unexpected coefPermyriad=%d - layout mismatch, editing disabled", c);
        } else {
            bool atkL = is_local(self), tgtL = is_local(target);
            float rate = cfg::g.damage_rate.load(std::memory_order_relaxed);
            bool god = cfg::g.god_mode.load(std::memory_order_relaxed);
            int32_t nc = c;
            if (god && tgtL) nc = 0;
            else if (rate != 1.0f && atkL && !tgtL) nc = (int32_t)std::min((double)c * rate, 2.0e9);
            if (g_logs.fetch_add(1) < 12) LOGI("battle: SendDamage attackerLocal=%d targetLocal=%d coef %d -> %d", atkL, tgtL, c, nc);
            if (nc != c) *coef = nc;
        }
    }
    orig_send(self, atk, target, reaction, dsrc, last, popup, fx, mi);
}
}  // namespace

bool features::install_battle(bool dry_run) {
    il2cpp::MethodRef send;
    if (!il2cpp::resolve("Btl", "CharacterUnit", "SendDamage", 7, "System.Void",
                         {"System.Int32", "Btl.CharacterUnit", "DamageReaction", "Btl.Logic.DamageSource",
                          "System.Boolean", "System.Boolean", "System.Boolean"}, send)) return false;
    void* cu = il2cpp::find_class("Btl", "CharacterUnit");
    void* tm = il2cpp::find_class("Btl", "TeamManagerUnit");
    void* ds = il2cpp::find_class("Btl.Logic", "DamageSource");
    if (!cu || !tm || !ds) { LOGW("NOT FOUND battle classes (CharacterUnit/TeamManagerUnit/DamageSource)"); return false; }
    if (!il2cpp::instance_field(cu, "CharacterUnit", "<teamManager>k__BackingField", "Btl.TeamManagerUnit", &off_tm)) return false;
    if (!il2cpp::instance_field(tm, "TeamManagerUnit", "<isLocalTeam>k__BackingField", "System.Boolean", &off_local)) return false;
    void* f = il2cpp::api.class_get_field_from_name ? il2cpp::api.class_get_field_from_name(ds, "coefPermyriad") : nullptr;
    if (!f || il2cpp::type_name(il2cpp::api.field_get_type(f)) != "System.Int32" || il2cpp::api.field_get_offset(f) < 0x10) {
        LOGW("NOT FOUND DamageSource.coefPermyriad (System.Int32)"); return false;
    }
    off_coef = il2cpp::api.field_get_offset(f) - 0x10;   // valuetype field offsets include the 0x10 object header
    LOGI("battle: offsets teamManager=0x%zx isLocalTeam=0x%zx coefPermyriad=0x%zx", off_tm, off_local, off_coef);
    if (dry_run) return true;
    return install_hook("battle(SendDamage)", send.addr, reinterpret_cast<void*>(&hook_send), reinterpret_cast<void**>(&orig_send), true);
}
