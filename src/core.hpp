#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace r6 {
// Names below are analyst-assigned. Layout follows the executable's raw writes.
struct Operator {
    char name[32]{};
    float vertical = 10.0f;
    float horizontal = 0.0f;
    std::uint8_t rapid = 0;
    std::uint8_t padding0[3]{};
    std::int32_t delay = 50;
    std::uint8_t humanize = 1;
    std::uint8_t padding1[3]{};
};
static_assert(sizeof(Operator) == 52);
static_assert(offsetof(Operator, vertical) == 32);
static_assert(offsetof(Operator, rapid) == 40);
static_assert(offsetof(Operator, delay) == 44);
static_assert(offsetof(Operator, humanize) == 48);
constexpr int operator_count = 72;
// The original writes exactly 71 operator records; extension operators (Noor)
// keep r6_config.bin at 3,734 bytes and live in azec_weapons.json instead.
constexpr int legacy_operator_count = 71;
// Bindable keys reject mouse buttons, lone modifiers, and Windows keys.
inline bool validHotkey(int key){
    if(key==0)return true;
    if(key<3||key>255)return false;
    if(key>=4&&key<=6)return false;
    if(key>=16&&key<=18)return false;
    if(key==91||key==92)return false;
    if(key>=160&&key<=165)return false;
    return true;
}
// Global actions (show/hide, recoil, panic) must not eat ordinary typing or
// list-navigation keys. Weapon slots still accept 1/2 because they are meant
// for the game and the foreground gate protects other applications.
inline bool validGlobalHotkey(int key){
    if(key==0)return true;
    if(!validHotkey(key))return false;
    if(key==8||key==9||key==13||key==27||key==32)return false;       // Backspace, Tab, Enter, Esc, Space
    if(key>=48&&key<=57)return false;                                  // 0-9
    if(key>=65&&key<=90)return false;                                  // A-Z
    if(key>=186&&key<=192)return false;                                // OEM punctuation
    if(key>=219&&key<=222)return false;                                // brackets, quote, tilde
    return true;
}
struct Config {
    std::array<Operator, operator_count> operators;
    std::int32_t menu_key = 0x2d, recoil_key = 0x75, panic_key = 0x23;
    std::int32_t selected = 0, language = 1;
    std::uint8_t sound = 1;
    std::int32_t rapid_key = 0x77;
    std::uint8_t rapid_master = 1;
    float neon[3] = {0.0f, 1.0f, 0.4f};
    float radius = 130.0f;
    // Extensions live in azec_weapons.json; bytes() retains the original layout.
    std::array<Operator, operator_count> secondary;
    std::array<bool, operator_count> secondary_enabled{}, secondary_initialized{};
    int weapon_slot = 0, primary_key = '1', secondary_key = '2';
    bool phone_enabled = true;
    Config();
    int active_slot() const { return weapon_slot == 1 && secondary_enabled[selected] ? 1 : 0; }
    Operator& active_operator() { return active_slot() ? secondary[selected] : operators[selected]; }
    const Operator& active_operator() const { return active_slot() ? secondary[selected] : operators[selected]; }
    void enable_secondary(int id, bool enabled) {
        if (enabled && !secondary_initialized[id]) { secondary[id]=operators[id]; secondary_initialized[id]=true; }
        secondary_enabled[id]=enabled;
        if (id==selected && !enabled) weapon_slot=0;
    }
    bool load(const std::filesystem::path& path);
    bool save(const std::filesystem::path& path) const;
    std::vector<std::uint8_t> bytes() const;
};
struct Movement { int x, y; };
class RecoilAccumulator {
    float x_ = 0, y_ = 0;
public:
    Movement step(const Operator& op, int jitter);
    void reset() { x_ = y_ = 0; }
};
int recoil_interval(bool rapidActive);
int rapid_interval(const Operator& op);
}
