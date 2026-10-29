#include <jni.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include "config.h"
#include "log.h"
#include "worker.h"
#include "zygisk.hpp"  // official Zygisk module API header (the workflow downloads it)

namespace {
constexpr const char* kPackageFile = "/data/adb/modules/kuro_mirror_high/package.txt";
constexpr const char* kConfigName  = "kuro_mirror_high.cfg";

std::string read_package() {
    FILE* f = fopen(kPackageFile, "r");
    if (!f) return {};
    char line[256] = {0};
    std::string out;
    while (fgets(line, sizeof line, f)) {
        std::string s = line;
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) s.pop_back();
        while (!s.empty() && s.front() == ' ') s.erase(s.begin());
        if (s.empty() || s[0] == '#') continue;
        out = s; break;
    }
    fclose(f);
    return out;
}
}  // namespace

class SpeedModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api* api, JNIEnv* env) override { api_ = api; env_ = env; }

    void preAppSpecialize(zygisk::AppSpecializeArgs* args) override {
        std::string nice = jstr(args->nice_name);
        target_ = read_package();
        match_ = !target_.empty() && nice == target_;
        if (!match_) { api_->setOption(zygisk::Option::DLCLOSE_MODULE_LIBRARY); return; }
        std::string dir = jstr(args->app_data_dir);
        if (dir.empty()) dir = "/data/data/" + target_;
        config_path_ = dir + "/files/" + kConfigName;
    }

    void postAppSpecialize(const zygisk::AppSpecializeArgs*) override {
        if (!match_) return;
        std::string base = config_path_.substr(0, config_path_.find_last_of('/'));
        std::string ext = "/storage/emulated/0/Android/data/" + target_ + "/files/";
        klog_set_paths((base + "/kuro_mirror_high.log").c_str(), (ext + "kuro_mirror_high.log").c_str());
        cfg::set_path(config_path_);
        cfg::set_override_path(ext + kConfigName);
        cfg::ensure_default_file();
        cfg::reload_if_changed();
        LOGI("Kuro Mirror High v0.4 loaded into %s, stage=%d", target_.c_str(), cfg::g.stage.load());
        if (cfg::g.stage.load() == 0) { LOGI("stage 0: idle"); return; }
        std::thread(worker_main, config_path_).detach();
    }

private:
    std::string jstr(jstring s) {
        if (!s) return {};
        const char* c = env_->GetStringUTFChars(s, nullptr);
        std::string out = c ? c : "";
        if (c) env_->ReleaseStringUTFChars(s, c);
        return out;
    }
    zygisk::Api* api_ = nullptr;
    JNIEnv* env_ = nullptr;
    bool match_ = false;
    std::string target_, config_path_;
};

REGISTER_ZYGISK_MODULE(SpeedModule)
