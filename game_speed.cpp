#include <cstring>
#include "config.h"
#include "feature_hooks.h"
#include "hook_util.h"
#include "il2cpp_api.h"
#include "log.h"

// Game Speed: the game has its own battle speed property (get_BattleSpeedRate). The declaring class is
// not known statically, so every class with an instance get_BattleSpeedRate() that returns System.Single
// is found at runtime and wrapped: result = original * game_speed.
namespace {
constexpr int kMax = 4;
struct Slot { void* addr = nullptr; float (*orig)(void*, const void*) = nullptr; };
Slot slots[kMax];
int nslots = 0;
bool dry = false;

template <int N> float fn(void* self, const void* mi) {
    float r = slots[N].orig(self, mi);
    float f = cfg::g.game_speed.load(std::memory_order_relaxed);
    return f == 1.0f ? r : r * f;
}
using Tramp = float (*)(void*, const void*);
Tramp tramps[kMax] = {&fn<0>, &fn<1>, &fn<2>, &fn<3>};

bool pred(const char* n) { return strcmp(n, "get_BattleSpeedRate") == 0; }
void cb(void* klass, void* method, void* image) {
    const char* ns = il2cpp::api.class_get_namespace ? il2cpp::api.class_get_namespace(klass) : "";
    const char* cn = il2cpp::api.class_get_name ? il2cpp::api.class_get_name(klass) : "?";
    if (il2cpp::api.method_get_param_count && il2cpp::api.method_get_param_count(method) != 0) return;
    std::string ret = il2cpp::type_name(il2cpp::api.method_get_return_type(method));
    uint32_t flags = il2cpp::api.method_get_flags(method, nullptr);
    void* addr = *reinterpret_cast<void**>(method);
    LOGI("game_speed candidate: %s.%s::get_BattleSpeedRate -> %s [%s] static=%d addr=%p", ns ? ns : "", cn, ret.c_str(),
         il2cpp::image_name_of(image), (flags & 0x10) ? 1 : 0, addr);
    if (ret != "System.Single" || (flags & 0x10) || !addr) return;
    if (nslots >= kMax) { LOGW("game_speed: more than %d candidates, extra ignored", kMax); return; }
    if (dry) { ++nslots; return; }
    if (install_hook("game_speed(get_BattleSpeedRate)", addr, reinterpret_cast<void*>(tramps[nslots]),
                     reinterpret_cast<void**>(&slots[nslots].orig), true)) { slots[nslots].addr = addr; ++nslots; }
}
}  // namespace

bool features::install_game_speed(bool dry_run) {
    dry = dry_run; nslots = 0;
    if (!il2cpp::scan_methods(pred, cb)) return false;
    if (nslots == 0) { LOGW("NOT FOUND usable get_BattleSpeedRate (instance, returns System.Single)"); return false; }
    return true;
}
