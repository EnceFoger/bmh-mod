#include <cstring>
#include "config.h"
#include "feature_hooks.h"
#include "il2cpp_api.h"
#include "log.h"

// Diagnostic only (not a feature): when dump=1, lists speed/damage-related methods of the game assembly in the log.
namespace {
int logged = 0;
bool pred(const char* n) {
    static const char* skip[] = {"Popup", "Gauge", "Text", "Anim", "Sprite", "Icon", "Effect", "View", "Cell", "Button", "Sound", "Voice", "Log"};
    static const char* keys[] = {"Damage", "Immortal", "Invincible", "SetCurrentHp", "SetCurrentHP", "SetReserveHP", "IsDead",
                                 "IsDefeated", "TimeScale", "SpeedRate", "BattleSpeed", "GameSpeed"};
    bool hit = false;
    for (auto* k : keys) if (strstr(n, k)) { hit = true; break; }
    if (!hit) return false;
    for (auto* s : skip) if (strstr(n, s)) return false;
    return true;
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
    uint32_t fl = il2cpp::api.method_get_flags(method, nullptr);
    LOGI("dump: %s.%s::%s(%s) -> %s%s", ns ? ns : "", cn, il2cpp::api.method_get_name(method), sig.c_str(), ret.c_str(), (fl & 0x10) ? " [static]" : "");
    ++logged;
}

std::string sig_of(void* method) {
    uint32_t argc = il2cpp::api.method_get_param_count ? il2cpp::api.method_get_param_count(method) : 0;
    std::string sig;
    for (uint32_t i = 0; i < argc; ++i) {
        if (i) sig += ", ";
        sig += il2cpp::type_name(il2cpp::api.method_get_param(method, i));
        if (il2cpp::api.method_get_param_name) { sig += " "; const char* pn = il2cpp::api.method_get_param_name(method, i); sig += pn ? pn : "?"; }
    }
    return sig;
}

// Structure dump: fields (+ optionally all methods) of the battle classes the hooks would need.
void dump_class(void* k, bool methods) {
    auto& A = il2cpp::api;
    if (!A.class_get_fields || !A.field_get_name) return;
    void* par = A.class_get_parent ? A.class_get_parent(k) : nullptr;
    LOGI("dumpc: class %s.%s : %s.%s valuetype=%d size=%d", A.class_get_namespace(k), A.class_get_name(k),
         par ? A.class_get_namespace(par) : "", par ? A.class_get_name(par) : "-",
         A.class_is_valuetype ? (int)A.class_is_valuetype(k) : -1, A.class_instance_size ? (int)A.class_instance_size(k) : -1);
    void* it = nullptr;
    while (void* f = A.class_get_fields(k, &it)) {
        LOGI("dumpc:   field %s : %s @0x%zx%s", A.field_get_name(f), il2cpp::type_name(A.field_get_type(f)).c_str(),
             A.field_get_offset(f), (A.field_get_flags(f) & 0x10) ? " [static]" : "");
    }
    if (!methods) return;
    it = nullptr;
    while (void* m = A.class_get_methods(k, &it)) {
        uint32_t fl = A.method_get_flags(m, nullptr);
        LOGI("dumpc:   method %s(%s) -> %s%s", A.method_get_name(m), sig_of(m).c_str(),
             il2cpp::type_name(A.method_get_return_type(m)).c_str(), (fl & 0x10) ? " [static]" : "");
    }
}

void dump_structs() {
    auto& A = il2cpp::api;
    if (!A.image_get_class_count || !A.image_get_class || !A.class_get_methods) return;
    size_t n = 0;
    void** asms = A.domain_get_assemblies(A.domain_get(), &n);
    for (size_t i = 0; asms && i < n; ++i) {
        void* img = A.assembly_get_image(asms[i]);
        if (!img || strncmp(il2cpp::image_name_of(img), "Assembly-CSharp", 15) != 0) continue;
        int cc = A.image_get_class_count(img);
        for (int ci = 0; ci < cc; ++ci) {
            void* k = A.image_get_class(img, ci);
            if (!k) continue;
            const char* cn = A.class_get_name(k);
            const char* ns = A.class_get_namespace(k);
            if (!cn || !ns) continue;
            bool unit = !strcmp(cn, "CharacterUnit") && !strcmp(ns, "Btl");
            bool small = !strcmp(cn, "DamageReaction") || (!strcmp(cn, "DamageSource") && !strcmp(ns, "Btl.Logic")) ||
                         (!strcmp(cn, "TeamManagerUnit") && !strcmp(ns, "Btl"));
            if (!unit && !small) continue;
            dump_class(k, unit);
            if (unit && A.class_get_parent) {   // inherited fields (HP/team may live in a base class)
                for (void* p = A.class_get_parent(k); p; p = A.class_get_parent(p)) {
                    const char* pn = A.class_get_name(p);
                    if (!pn || !strcmp(pn, "Object") || !strcmp(pn, "MonoBehaviour") || !strcmp(pn, "ValueType")) break;
                    dump_class(p, false);
                }
            }
        }
    }
}
}  // namespace

void features::dump_candidates() {
    if (!cfg::g.dump.load()) return;
    logged = 0;
    il2cpp::scan_methods(pred, cb, 2000);
    LOGI("dump: %d methods logged (diagnostic, nothing hooked)", logged);
    dump_structs();
    LOGI("dumpc: done");
}
