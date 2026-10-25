#pragma once
#include "core.hpp"
#include <QByteArray>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace r6 {
struct Hotkeys {
    int menu=0, recoil=0, panic=0, primary=0, secondary=0;
};
class Engine {
    mutable std::mutex config_mutex_;
    Config config_;
    std::mutex stop_mutex_;
    std::condition_variable stop_condition_;
    std::thread recoil_, rapid_;
    bool dry_run_;
    void sleep(int ms);
    void input(unsigned long flags, int x=0, int y=0);
    std::pair<Operator,int> current_profile() const;
    void recoil_worker();
    void rapid_worker();
public:
    std::atomic_bool running{false}, recoil_active{false};
    std::atomic_bool game_focus_only{true}, ads_only{false}, input_allowed{false};
    std::atomic_bool menu_visible{true};
    std::atomic_uint injected_events{0};
    explicit Engine(bool dry_run);
    ~Engine();
    Config snapshot() const;
    Hotkeys hotkeys() const;
    void edit(const std::function<void(Config&)>& change);
    bool select_weapon(int slot);
    bool rapid_active() const;
    void set_rapid_active(bool on);
    bool load(const std::filesystem::path& file,bool* weaponProfilesValid=nullptr);
    bool save(const std::filesystem::path& file,const QByteArray& uiPreferences={}) const;
    void start();
    void stop();
    bool dry_run() const { return dry_run_; }
};
}
