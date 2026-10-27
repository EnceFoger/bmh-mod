#include <atomic>
#include <unistd.h>
#include "config.h"
#include "feature_hooks.h"
#include "hook_util.h"
#include "il2cpp_api.h"
#include "log.h"

// Unity Speed: scales every Time.timeScale the game requests (pause = 0 stays 0).
// Slider/config changes are applied from a Time.get_deltaTime hook on the thread that set timeScale.
namespace {
void  (*orig_set)(float, const void*) = nullptr;
float (*orig_dt)(const void*) = nullptr;
const void* set_mi = nullptr;
std::atomic<float>    g_last{1.0f};
std::atomic<int>      g_main_tid{0};
std::atomic<unsigned> g_applied_rev{0};

void hook_set(float value, const void* mi) {
    g_last.store(value, std::memory_order_relaxed);
    g_main_tid.store(gettid(), std::memory_order_relaxed);
    orig_set(value * cfg::g.unity_speed.load(std::memory_order_relaxed), mi);
}
float hook_dt(const void* mi) {
    float r = orig_dt(mi);
    int tid = g_main_tid.load(std::memory_order_relaxed);
    if (tid == 0 || gettid() != tid) return r;
    unsigned rev = cfg::g.revision.load(std::memory_order_relaxed);
    if (rev != g_applied_rev.load(std::memory_order_relaxed)) {
        g_applied_rev.store(rev, std::memory_order_relaxed);
        orig_set(g_last.load(std::memory_order_relaxed) * cfg::g.unity_speed.load(std::memory_order_relaxed), set_mi);
    }
    return r;
}
}  // namespace

bool features::install_unity_speed(bool dry_run) {
    il2cpp::MethodRef set, dt;
    if (!il2cpp::resolve("UnityEngine", "Time", "set_timeScale", 1, "System.Void", {"System.Single"}, set, true)) return false;
    bool have_dt = il2cpp::resolve("UnityEngine", "Time", "get_deltaTime", 0, "System.Single", {}, dt, true);
    if (dry_run) { LOGI("probe: unity_speed targets OK (not hooked)"); return true; }
    set_mi = set.info;
    if (!install_hook("unity_speed(set_timeScale)", set.addr, reinterpret_cast<void*>(&hook_set),
                      reinterpret_cast<void**>(&orig_set), true)) return false;
    if (!have_dt || !install_hook("unity_speed(get_deltaTime)", dt.addr, reinterpret_cast<void*>(&hook_dt),
                                  reinterpret_cast<void**>(&orig_dt), true))
        LOGW("unity_speed: config changes apply only when the game next sets Time.timeScale");
    return true;
}
