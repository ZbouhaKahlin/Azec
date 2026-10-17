#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>

namespace r6 {
// FUN_140063d90 constructs 73 names but stops after index 70. Zero and Zofia
// are embedded strings, but never enter the original's 71-element list.
static const char* names[] = {
    "Ace", "Alibi", "Amaru", "Aruni", "Ash", "Azami", "Bandit", "Blackbeard",
    "Blitz", "Brava", "Buck", "Capitao", "Castle", "Caveira", "Clash", "Deimos",
    "Doc", "Dokkaebi", "Echo", "Ela", "Fenrir", "Finka", "Flores", "Frost",
    "Fuze", "Glaz", "Goyo", "Gridlock", "Grim", "Hibana", "Iana", "IQ", "Jackal",
    "Jager", "Kaid", "Kali", "Kapkan", "Lesion", "Lion", "Maestro", "Maverick",
    "Melusi", "Mira", "Montagne", "Mozzie", "Mute", "Nokk", "Osa", "Oryx",
    "Pulse", "Ram", "Rook", "Sens", "Sentry", "Skopos", "Smoke", "Solis",
    "Striker", "Sledge", "Tachanka", "Thatcher", "Thermite", "Thorn",
    "Thunderbird", "Tubarao", "Twitch", "Valkyrie", "Vigil", "Wamai", "Warden", "Ying"
};
static_assert(std::size(names) == operator_count);
Config::Config() {
    for (int i = 0; i < operator_count; ++i)
        std::strncpy(operators[i].name, names[i], sizeof(operators[i].name) - 1);
    secondary=operators;
}
template<class T> static void append(std::vector<std::uint8_t>& b, const T& value) {
    const auto* p = reinterpret_cast<const std::uint8_t*>(&value);
    b.insert(b.end(), p, p + sizeof(T));
}
std::vector<std::uint8_t> Config::bytes() const {
    std::vector<std::uint8_t> b;
    b.reserve(3734);
    append(b, operators); append(b, menu_key); append(b, recoil_key);
    append(b, panic_key); append(b, selected); append(b, language); append(b, sound);
    append(b, rapid_key); append(b, rapid_master); append(b, neon); append(b, radius);
    return b;
}
bool Config::save(const std::filesystem::path& path) const {
    const auto b = bytes();
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    f.write(reinterpret_cast<const char*>(b.data()), b.size());
    return bool(f);
}
bool Config::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    const std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(f)), {});
    // The original reads partial records unchecked. Reject malformed core data
    // to prevent invalid names, NaNs, or an out-of-range worker index.
    if (b.size() < 3708) return false;
    Config next;
    std::size_t offset = 0;
    auto read = [&](auto& value) {
        if (offset + sizeof(value) > b.size()) return false;
        std::memcpy(&value, b.data() + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    };
    read(next.operators); read(next.menu_key); read(next.recoil_key);
    read(next.panic_key); read(next.selected);
    read(next.language); read(next.sound);
    const auto default_rapid_key = next.rapid_key;
    read(next.rapid_key);
    if (!next.rapid_key) next.rapid_key = default_rapid_key;
    read(next.rapid_master); read(next.neon); read(next.radius);
    if (next.selected < 0 || next.selected >= operator_count) return false;
    for (auto& op : next.operators) {
        if (!std::memchr(op.name, 0, 32) || !std::isfinite(op.vertical) ||
            !std::isfinite(op.horizontal)) return false;
        if (op.vertical < 0 || op.vertical > 100 || op.horizontal < -50 ||
            op.horizontal > 50 || op.delay < 0 || op.delay > 60000) return false;
        op.rapid = !!op.rapid; op.humanize = !!op.humanize;
    }
    if (next.language < 0 || next.language > 1 || !std::isfinite(next.radius) ||
        next.radius < 1 || next.radius > 10000) return false;
    for (auto c : next.neon) if (!std::isfinite(c) || c < 0 || c > 1) return false;
    for (auto key : {next.menu_key,next.recoil_key,next.panic_key,next.rapid_key})
        if (key < 0 || key > 255) return false;
    *this = next;
    return true;
}
Movement RecoilAccumulator::step(const Operator& op, int jitter) {
    x_ = float(jitter) + op.horizontal + x_;
    y_ = float(jitter) + op.vertical + y_;
    Movement movement{int(x_), int(y_)};
    x_ -= float(movement.x); y_ -= float(movement.y);
    return movement;
}
int recoil_interval(const Operator& op, bool master) { return master && op.rapid ? 30 : 12; }
int rapid_interval(const Operator& op) { return std::max(30, op.delay); }
bool self_test(const std::filesystem::path& output) {
    Config config;
    const auto check = [](bool ok, const char* name) {
        std::cout << (ok ? "PASS " : "FAIL ") << name << '\n';
        return ok;
    };
    bool ok = check(sizeof(Operator) == 52 && config.bytes().size() == 3734, "recovered binary layout");
    ok &= check(std::string(config.operators.front().name) == "Ace" &&
                std::string(config.operators.back().name) == "Ying", "original operator bounds");
    RecoilAccumulator accumulator;
    Operator op; op.vertical = 1.25f; op.horizontal = -0.5f;
    int x = 0, y = 0;
    for (int i=0;i<4;++i) { const auto m=accumulator.step(op,0); x+=m.x; y+=m.y; }
    ok &= check(x == -2 && y == 5, "fractional signed movement accumulation");
    accumulator.reset();
    auto m = accumulator.step(op, -1);
    ok &= check(m.x == -1 && m.y == 0, "shared signed humanization jitter");
    op.rapid=1; op.delay=10;
    ok &= check(recoil_interval(op,true)==30 && recoil_interval(op,false)==12 &&
                rapid_interval(op)==30, "worker timing and delay floor");
    config.operators[4].horizontal=-4.5f;
    config.selected=4; config.language=1; config.neon[0]=0.25f;
    Config loaded;
    ok &= check(config.save(output) && loaded.load(output) &&
                config.bytes()==loaded.bytes(), "configuration byte round trip");
    config.enable_secondary(4,true);config.weapon_slot=1;config.active_operator().vertical=23.5f;
    ok &= check(config.active_operator().vertical==23.5f&&config.operators[4].vertical==10&&config.bytes().size()==3734,"secondary profile isolation and legacy layout");
    config.enable_secondary(4,false);
    ok &= check(config.active_slot()==0&&config.active_operator().vertical==10,"disabled secondary falls back to primary");
    config.enable_secondary(4,true);config.weapon_slot=1;
    ok &= check(config.active_operator().vertical==23.5f,"secondary edits survive disabling and reenabling");
    std::cout << "CONFIG " << output.string() << '\n';
    return ok;
}
}
