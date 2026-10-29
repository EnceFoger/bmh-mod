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
    "stage=3                 # 3 = full (default) | 0 idle | 1 find API | 2 find methods, no hooks\n"
    "unity_speed=1.0         # all Time.timeScale requests (menus included)\n"
    "damage_rate=1.0        # damage you deal x this (1.0 = off)\n"
    "god_mode=0             # 1 = your team takes no damage\n"
    "max_damage_rate=10.0   # upper limit for damage_rate (max 100)\n"
    "dump=0                  # diagnostic: 1 = write speed/damage-related game methods to the log (no effect on gameplay)\n"
    "max_unity_speed=5.0     # raise this (max 100) to allow unity_speed above 5\n"
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

static void mkdirs(const std::string& file_path) {
    size_t sl = file_path.find_last_of('/');
    if (sl == std::string::npos) return;
    std::string dir = file_path.substr(0, sl);
    for (size_t i = 1; i <= dir.size(); ++i)
        if (i == dir.size() || dir[i] == '/') mkdir(dir.substr(0, i).c_str(), 0771);   // EEXIST is fine
}
static bool make_default(const std::string& p) {
    if (p.empty()) return false;
    mkdirs(p);
    std::ofstream f(p);
    if (!f) return false;
    f << kDefaultFile;
    f.flush();
    if (!f) return false;
    LOGI("created default config: %s", p.c_str());
    return true;
}

// Creates the default config when none exists yet: first in the user-editable external folder
// (/sdcard/Android/data/<pkg>/files), otherwise in the app's internal folder.
void ensure_default_file() {
    if ((!s_over.empty() && access(s_over.c_str(), F_OK) == 0) || (!s_path.empty() && access(s_path.c_str(), F_OK) == 0)) return;
    if (make_default(s_over)) return;
    if (!make_default(s_path)) LOGW("could not create default config (external: %s, internal: %s)", s_over.c_str(), s_path.c_str());
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

    g.stage = 3; g.unity_speed = 1.f; g.damage_rate = 1.f; g.god_mode = false; g.dump = false;
    g.max_unity_speed = kDefaultMax; g.max_damage_rate = 10.0f;
    std::string line;
    while (std::getline(in, line)) {
        size_t h = line.find('#'); if (h != std::string::npos) line.erase(h);
        size_t eq = line.find('='); if (eq == std::string::npos) continue;
        std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
        float f; bool b;
        if      (k == "unity_speed" && parse_float(v, f)) g.unity_speed = f;
        else if (k == "damage_rate" && parse_float(v, f)) g.damage_rate = f;
        else if (k == "god_mode" && parse_bool(v, b)) g.god_mode = b;
        else if (k == "max_damage_rate") { float m; if (parse_float(v, m) && m >= 1.f) g.max_damage_rate = m; }
        else if ((k == "dump" || k == "damage_dump") && parse_bool(v, b)) g.dump = b;
        else if (k == "max_unity_speed") g.max_unity_speed = parse_max(k, v);
        else if (k == "stage") { char* e = nullptr; long x = strtol(v.c_str(), &e, 10); if (e != v.c_str()) g.stage = (int)std::min(3L, std::max(0L, x)); }
        else LOGW("config: ignored line '%s=%s'", k.c_str(), v.c_str());
    }
    clamp_to(g.unity_speed, g.max_unity_speed.load());
    clamp_to(g.damage_rate, g.max_damage_rate.load());
    g.revision.fetch_add(1);
    LOGI("config loaded: stage=%d unity_speed=%.1f damage_rate=%.1f god_mode=%d dump=%d",
         g.stage.load(), g.unity_speed.load(), g.damage_rate.load(), (int)g.god_mode.load(), (int)g.dump.load());
}
}  // namespace cfg
