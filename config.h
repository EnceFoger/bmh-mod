#pragma once
#include <atomic>
#include <string>

namespace cfg {
struct Settings {
    std::atomic<float> unity_speed{1.0f};   // 1.0 = off. Scales every Time.timeScale the game requests
    std::atomic<float> game_speed{1.0f};    // 1.0 = off. Scales the game's own BattleSpeedRate
    std::atomic<float> damage_rate{1.0f};   // 1.0 = off. (hook only after discovery, see README)
    std::atomic<bool>  damage_dump{true};   // log damage-related methods once (discovery)
    std::atomic<float> max_unity_speed{5.0f};
    std::atomic<float> max_game_speed{5.0f};
    std::atomic<float> max_damage_rate{5.0f};
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
