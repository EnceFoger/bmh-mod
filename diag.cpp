#include <cstring>
#include "config.h"
#include "feature_hooks.h"
#include "il2cpp_api.h"
#include "log.h"

// Diagnostic only (not a feature): when dump=1, lists speed/damage-related methods of the game assembly in the log.
namespace {
int logged = 0;
bool pred(const char* n) {
    static const char* names[] = {"SendDamage", "OnReceivedDamage", "ReceivedDamage", "RegisterDamage", "SetDamageValue",
                                  "OnReceiveNormalDamage", "CalcRate", "ApplyDamageReaction"};
    for (auto* s : names) if (strcmp(n, s) == 0) return true;
    return strstr(n, "TimeScale") || strstr(n, "SpeedRate") || strstr(n, "BattleSpeed") || strstr(n, "GameSpeed");
}
void cb(void* klass, void* method, void* image) {
    if (strncmp(il2cpp::image_name_of(image), "Assembly-CSharp", 15) != 0) return;
    const char* ns = il2cpp::api.class_get_namespace ? il2cpp::api.class_get_namespace(klass) : "";
    const char* cn = il2cpp::api.class_get_name ? il2cpp::api.class_get_name(klass) : "?";
    uint32_t argc = il2cpp::api.method_get_param_count ? il2cpp::api.method_get_param_count(method) : 0;
    std::string sig;
    for (uint32_t i = 0; i < argc; ++i) {
        if (i) sig += ", ";
        sig += il2cpp::type_name(il2cpp::api.method_get_param(method, i));
        if (il2cpp::api.method_get_param_name) { sig += " "; const char* pn = il2cpp::api.method_get_param_name(method, i); sig += pn ? pn : "?"; }
    }
    std::string ret = il2cpp::type_name(il2cpp::api.method_get_return_type(method));
    LOGI("dump: %s.%s::%s(%s) -> %s", ns ? ns : "", cn, il2cpp::api.method_get_name(method), sig.c_str(), ret.c_str());
    ++logged;
}
}  // namespace

void features::dump_candidates() {
    if (!cfg::g.dump.load()) return;
    logged = 0;
    il2cpp::scan_methods(pred, cb, 2000);
    LOGI("dump: %d methods logged (diagnostic, nothing hooked)", logged);
}
