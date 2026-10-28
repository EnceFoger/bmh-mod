#include "config.h"
#include "log.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

namespace cfg {
Settings g;
static const float kHardMax = 100.0f, kDefaultMax = 5.0f, kMin = 0.1f;
static std::string s_path, s_over, s_used;
static long long s_stamp = -1;

static const char* kDefaultFile =
    "# Kuro Mirror High settings. Re-read automatically every ~2 seconds.\n"
    "# 1.0 = feature off.\n"
    "stage=1                 # 0 idle | 1 find API | 2 find methods, no hooks | 3 full (set 3 to use features)\n"
    "unity_speed=1.0         # all Time.timeScale requests (menus included)\n"
    "game_speed=1.0          # game's own battle speed rate (BattleSpeedRate)\n"
    "damage_dump=0           # diagnostic: 1 = write damage-related method list to the log (no effect on gameplay)\n"
    "max_unity_speed=5.0\n"
    "max_game_speed=5.0\n"
    ;

void set_path(const std::string& p) { s_path = p; }
void set_override_path(const std::string& p) { s_over = p; }
static std::string active_path() { return (!s_over.empty() && access(s_over.c_str(), R_OK) == 0) ? s_over : s_path; }

static std::string trim(std::string s) {
    auto sp = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && sp(s.front())) s.erase(s.begin());
    while (!s.empty() && sp(s.back())) s.pop_back();
    return s;
}
static bool parse_float(const std::string& v, float& out) {
    char* e = nullptr; float f = strtof(v.c_str(), &e);
    if (e == v.c_str() || !std::isfinite(f) || f < 0.f) return false;
    out = std::min(kHardMax, f); return true;
}
static bool parse_bool(const std::string& v, bool& out) {
    if (v == "1" || v == "true" || v == "on") { out = true; return true; }
    if (v == "0" || v == "false" || v == "off") { out = false; return true; }
    return false;
}
static float parse_max(const std::string& k, const std::string& v) {
    float f; if (parse_float(v, f) && f >= 1.f) return f;
    LOGW("config: invalid %s='%s' -> default %.1f", k.c_str(), v.c_str(), kDefaultMax); return kDefaultMax;
}
static void clamp_to(std::atomic<float>& v, float lim) { v.store(std::max(kMin, std::min(lim, v.load()))); }

void ensure_default_file() {
    if (s_path.empty() || access(s_path.c_str(), F_OK) == 0) return;
    size_t sl = s_path.find_last_of('/');
    if (sl != std::string::npos) mkdir(s_path.substr(0, sl).c_str(), 0771);
    std::ofstream f(s_path);
    if (f) { f << kDefaultFile; LOGI("created default config: %s", s_path.c_str()); }
    else LOGW("could not create default config at %s", s_path.c_str());
}

void reload_if_changed() {
    struct stat st;
    std::string path = active_path();
    if (path != s_used) { s_used = path; s_stamp = -1; LOGI("config file in use: %s", path.c_str()); }
    if (path.empty() || stat(path.c_str(), &st) != 0) return;
    long long stamp = (long long)st.st_mtim.tv_sec * 1000000000LL + st.st_mtim.tv_nsec;
    if (stamp == s_stamp) return;
    std::ifstream in(path); if (!in) return;
    s_stamp = stamp;

    g.unity_speed = 1.f; g.game_speed = 1.f; g.damage_dump = false;
    g.max_unity_speed = g.max_game_speed = kDefaultMax;
    std::string line;
    while (std::getline(in, line)) {
        size_t h = line.find('#'); if (h != std::string::npos) line.erase(h);
        size_t eq = line.find('='); if (eq == std::string::npos) continue;
        std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
        float f; bool b;
        if      (k == "unity_speed" && parse_float(v, f)) g.unity_speed = f;
        else if (k == "game_speed"  && parse_float(v, f)) g.game_speed = f;
        else if (k == "damage_dump" && parse_bool(v, b))  g.damage_dump = b;
        else if (k == "max_unity_speed") g.max_unity_speed = parse_max(k, v);
        else if (k == "max_game_speed")  g.max_game_speed = parse_max(k, v);
        else if (k == "stage") { char* e = nullptr; long x = strtol(v.c_str(), &e, 10); if (e != v.c_str()) g.stage = (int)std::min(3L, std::max(0L, x)); }
        else LOGW("config: ignored line '%s=%s'", k.c_str(), v.c_str());
    }
    clamp_to(g.unity_speed, g.max_unity_speed.load());
    clamp_to(g.game_speed, g.max_game_speed.load());
    g.revision.fetch_add(1);
    LOGI("config loaded: stage=%d unity_speed=%.1f game_speed=%.1f damage_dump=%d",
         g.stage.load(), g.unity_speed.load(), g.game_speed.load(), (int)g.damage_dump.load());
}
}  // namespace cfg
