#pragma once
#include <atomic>
#include <string>

namespace cfg {
struct Settings {
    std::atomic<float> unity_speed{1.0f};   // 1.0 = off. Scales every Time.timeScale the game requests
    std::atomic<bool>  dump{false};         // diagnostic only: log speed/damage-related game methods once
    std::atomic<float> max_unity_speed{5.0f};
    std::atomic<float> damage_multiplier{1.0f}; // 1.0 = off. Scales damage coefficient of hits landing on the enemy side
    std::atomic<bool>  god_mode{false};         // true = hits on the local team are ignored, local team is never "dead"
    std::atomic<unsigned> revision{0};
    // 0 idle | 1 find libil2cpp + API | 2 + find/probe methods, NO hooks | 3 full
    std::atomic<int> stage{1};
};
extern Settings g;
void set_path(const std::string& path);
void set_override_path(const std::string& path);
void ensure_default_file();
void reload_if_changed();
}  // namespace cfg
