#include "worker.h"
#include <chrono>
#include <thread>
#include "config.h"
#include "feature_hooks.h"
#include "il2cpp_api.h"
#include "log.h"

static void idle_loop() {
    for (;;) { std::this_thread::sleep_for(std::chrono::seconds(2)); cfg::reload_if_changed(); }
}
static void report(const char* name, bool ok, bool dry) {
    if (ok) LOGI("feature %-12s : %s", name, dry ? "FOUND (probe, not hooked)" : "OK");
    else    LOGW("feature %-12s : NOT FOUND / not hooked", name);
}

void worker_main(std::string config_path) {
    using namespace std::chrono;
    const int stage = cfg::g.stage.load();
    LOGI("worker started, stage=%d, config=%s", stage, config_path.c_str());
    std::this_thread::sleep_for(seconds(5));
    il2cpp::LibRef lib;
    for (int i = 0; i < 360 && !il2cpp::locate_library(lib); ++i) std::this_thread::sleep_for(milliseconds(500));
    if (!lib.dynamic) { LOGE("libil2cpp.so never loaded"); return idle_loop(); }
    if (!il2cpp::load_api(lib)) { LOGE("IL2CPP API exports unavailable - nothing hooked"); return idle_loop(); }
    LOGI("IL2CPP API resolved by name");
    if (stage < 2) { LOGI("stage 1 done (no hooks)"); return idle_loop(); }
    if (!il2cpp::wait_until_ready(180)) { LOGE("IL2CPP not ready"); return idle_loop(); }
    const bool dry = stage < 3;
    report("unity_speed", features::install_unity_speed(dry), dry);
    report("battle",      features::install_battle(dry), dry);
    features::dump_candidates();
    LOGI(dry ? "stage 2 done: probe only" : "stage 3 done: hooks installed");
    idle_loop();
}
