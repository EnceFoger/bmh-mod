#include <cstring>
#include "config.h"
#include "feature_hooks.h"
#include "il2cpp_api.h"
#include "log.h"

// Diagnostic only (not a feature): lists damage-related methods in the log when damage_dump=1.
// The metadata has several damage-related methods (SendDamage, OnReceivedDamage, RegisterDamage,
// SetDamageValue, ...) but which one carries the final damage number, and with what parameter types,
// cannot be verified without running the game. A wrong hook would crash or do nothing, so nothing is
// hooked: this logs every matching method with class, return type and parameter types/names so the
// correct single target can be wired in from the log.
namespace {
int logged = 0;
bool pred(const char* n) {
    static const char* names[] = {"SendDamage", "OnReceivedDamage", "ReceivedDamage", "ReceivedHitDamage", "RegisterDamage",
                                  "SetDamageValue", "OnReceiveNormalDamage", "ReciveDamage", "CalcRate", "ApplyDamageReaction"};
    for (auto* s : names) if (strcmp(n, s) == 0) return true;
    return false;
}
void cb(void* klass, void* method, void* image) {
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
    LOGI("damage candidate: [%s] %s.%s::%s(%s) -> %s", il2cpp::image_name_of(image), ns ? ns : "", cn,
         il2cpp::api.method_get_name(method), sig.c_str(), ret.c_str());
    ++logged;
}
}  // namespace

void features::dump_damage_candidates() {
    if (!cfg::g.damage_dump.load()) return;
    logged = 0;
    if (!il2cpp::scan_methods(pred, cb, 400)) return;
    LOGI("damage_dump: %d candidates logged (diagnostic, nothing hooked)", logged);
}
