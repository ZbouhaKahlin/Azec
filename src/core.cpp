#include "core.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>

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
    static std::atomic_uint sequence{0};
    auto temporary=path;
    temporary+=L".tmp-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(sequence++);
    const HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    DWORD written=0;
    bool ok=WriteFile(file,b.data(),DWORD(b.size()),&written,nullptr)&&written==b.size()&&FlushFileBuffers(file);
    if(!CloseHandle(file))ok=false;
    if(ok)ok=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
    if(!ok){const DWORD error=GetLastError();DeleteFileW(temporary.c_str());SetLastError(error);}
    return ok;
}
bool Config::load(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(0,std::ios::end);const auto size=f.tellg();
    if(size<3708||size>1024*1024)return false;
    f.seekg(0);
    const std::vector<std::uint8_t> b((std::istreambuf_iterator<char>(f)), {});
    // The original reads partial records unchecked. Reject malformed core data
    // to prevent invalid names, NaNs, or an out-of-range worker index.
    if (b.size() < 3708) return false;
    Config next;
    std::size_t offset = 0;
    auto read = [&](auto& value) {
        if (offset + sizeof(value) > b.size()) {offset=b.size();return false;}
        std::memcpy(&value, b.data() + offset, sizeof(value));
        offset += sizeof(value);
        return true;
    };
    read(next.operators); read(next.menu_key); read(next.recoil_key);
    read(next.panic_key); read(next.selected);
    read(next.language); read(next.sound);
    read(next.rapid_key);
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
}
