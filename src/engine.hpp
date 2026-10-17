#pragma once
#include "core.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace r6 {
class Engine {
    mutable std::mutex config_mutex_;
    Config config_;
    std::mutex stop_mutex_;
    std::condition_variable stop_condition_;
    std::thread recoil_, rapid_;
    void* logitech_ = nullptr;
    bool dry_run_;
    void sleep(int ms);
    void input(unsigned long flags, int x=0, int y=0);
    std::pair<Operator,int> current_profile() const;
    void recoil_worker();
    void rapid_worker();
public:
    std::atomic_bool running{false}, recoil_active{false}, recoil_master{true};
    std::atomic_bool rapid_master{true}, menu_visible{true};
    std::atomic_uint injected_events{0};
    explicit Engine(bool dry_run);
    ~Engine();
    Config snapshot() const;
    void edit(const std::function<void(Config&)>& change);
    void set_rapid_master(bool enabled);
    bool select_weapon(int slot);
    bool load(const std::filesystem::path& file);
    bool save(const std::filesystem::path& file) const;
    void start();
    void stop();
    bool dry_run() const { return dry_run_; }
};
}
