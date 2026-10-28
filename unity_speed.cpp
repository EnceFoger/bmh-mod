#include <atomic>
#include <cstring>
#include <sys/prctl.h>
#include <unistd.h>
#include "config.h"
#include "feature_hooks.h"
#include "hook_util.h"
#include "il2cpp_api.h"
#include "log.h"

// Unity Speed: scales Time.timeScale (pause = 0 stays 0).
// - Every game call to set_timeScale is multiplied by unity_speed.
// - Independently of that, the factor is applied from the Unity main thread (via a get_deltaTime hook) at start and
//   whenever the config changes, so it works even if the game never calls set_timeScale again.
namespace {
void  (*orig_set)(float, const void*) = nullptr;
float (*orig_dt)(const void*) = nullptr;
float (*get_ts)(const void*) = nullptr;
const void* set_mi = nullptr;
const void* get_ts_mi = nullptr;
std::atomic<float>    g_last{-1.0f};   // timeScale the game asked for (unscaled); <0 = unknown yet
std::atomic<int>      g_main_tid{0};
std::atomic<unsigned> g_applied_rev{0};
std::atomic<int>      g_set_logs{0};

bool is_unity_main() {
    char n[32] = {0};
    prctl(PR_GET_NAME, reinterpret_cast<unsigned long>(n), 0, 0, 0);
    return strcmp(n, "UnityMain") == 0;
}

void hook_set(float value, const void* mi) {
    g_last.store(value, std::memory_order_relaxed);
    g_main_tid.store(gettid(), std::memory_order_relaxed);
    float f = cfg::g.unity_speed.load(std::memory_order_relaxed);
    if (g_set_logs.fetch_add(1) < 5) LOGI("unity_speed: game set timeScale=%.2f -> %.2f", value, value * f);
    orig_set(value * f, mi);
}

float hook_dt(const void* mi) {
    float r = orig_dt(mi);
    int tid = g_main_tid.load(std::memory_order_relaxed);
    if (tid == 0) {
        if (!is_unity_main()) return r;
        tid = gettid();
        g_main_tid.store(tid, std::memory_order_relaxed);
        LOGI("unity_speed: Unity main thread detected (tid %d)", tid);
    }
    if (gettid() != tid) return r;
    unsigned rev = cfg::g.revision.load(std::memory_order_relaxed);
    if (rev == g_applied_rev.load(std::memory_order_relaxed)) return r;
    g_applied_rev.store(rev, std::memory_order_relaxed);
    float base = g_last.load(std::memory_order_relaxed);
    if (base < 0.0f) {
        if (!get_ts) return r;
        base = get_ts(get_ts_mi);
        g_last.store(base, std::memory_order_relaxed);
    }
    float f = cfg::g.unity_speed.load(std::memory_order_relaxed);
    LOGI("unity_speed: applied timeScale %.2f x %.2f = %.2f", base, f, base * f);
    orig_set(base * f, set_mi);
    return r;
}
}  // namespace

bool features::install_unity_speed(bool dry_run) {
    il2cpp::MethodRef set, dt, gts;
    if (!il2cpp::resolve("UnityEngine", "Time", "set_timeScale", 1, "System.Void", {"System.Single"}, set, true)) return false;
    bool have_dt = il2cpp::resolve("UnityEngine", "Time", "get_deltaTime", 0, "System.Single", {}, dt, true);
    bool have_gts = il2cpp::resolve("UnityEngine", "Time", "get_timeScale", 0, "System.Single", {}, gts, true);
    if (dry_run) { LOGI("probe: unity_speed targets OK (not hooked)"); return true; }
    set_mi = set.info;
    if (have_gts) { get_ts = reinterpret_cast<float (*)(const void*)>(gts.addr); get_ts_mi = gts.info; }
    if (!install_hook("unity_speed(set_timeScale)", set.addr, reinterpret_cast<void*>(&hook_set),
                      reinterpret_cast<void**>(&orig_set), true)) return false;
    if (!have_dt || !install_hook("unity_speed(get_deltaTime)", dt.addr, reinterpret_cast<void*>(&hook_dt),
                                  reinterpret_cast<void**>(&orig_dt), true))
        LOGW("unity_speed: get_deltaTime hook failed - speed applies only when the game sets Time.timeScale");
    return true;
}
