#include "core_checks.hpp"
#include <iostream>
#include <fstream>
#include <random>
#include <limits>
#include <cstring>
#include <algorithm>
#include <windows.h>

namespace r6 {
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
    const auto garbage=output.parent_path()/L"malformed-config.bin";
    const auto write=[&](const std::vector<std::uint8_t>& bytes){std::ofstream file(garbage,std::ios::binary|std::ios::trunc);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());return bool(file);};
    const auto original=Config{}.bytes();bool truncation=true;
    for(size_t size=0;size<3708;++size){const std::vector<std::uint8_t> bytes(original.begin(),original.begin()+size);Config target;const auto before=target.bytes();truncation&=write(bytes)&&!target.load(garbage)&&target.bytes()==before;}
    ok &= check(truncation,"truncated core rejected without changing existing config");
    bool legacy=true;
    for(size_t size=3708;size<=original.size();++size){Config target;legacy&=write({original.begin(),original.begin()+size})&&target.load(garbage)&&target.bytes().size()==3734;}
    ok &= check(legacy,"legacy optional tails remain compatible without misaligned reads");
    std::mt19937 random(20261017);bool fuzz=true;
    for(int i=0;i<128;++i){std::vector<std::uint8_t> bytes(random()%4097);for(auto& byte:bytes)byte=std::uint8_t(random());Config target;fuzz&=write(bytes)&&!target.load(garbage);}
    ok &= check(fuzz,"randomized invalid files rejected");
    auto bytes=original;std::fill(bytes.begin(),bytes.begin()+32,'X');Config invalid;
    ok &= check(write(bytes)&&!invalid.load(garbage),"unterminated name rejected");
    bytes=original;const float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(bytes.data()+32,&nan,sizeof(nan));
    ok &= check(write(bytes)&&!invalid.load(garbage),"nonfinite profile rejected");
    bytes=original;const std::int32_t outside=71;std::memcpy(bytes.data()+3704,&outside,sizeof(outside));
    ok &= check(write(bytes)&&!invalid.load(garbage),"invalid selected operator rejected");
    bytes.assign(1024*1024+1,0);
    ok &= check(write(bytes)&&!invalid.load(garbage),"oversized configuration rejected before allocation");
    Config old;old.operators[0].vertical=7.5f;Config replacement;replacement.operators[0].vertical=99;
    bool preserved=old.save(garbage);
    const auto locked=CreateFileW(garbage.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    preserved&=locked!=INVALID_HANDLE_VALUE&&!replacement.save(garbage);
    if(locked!=INVALID_HANDLE_VALUE)CloseHandle(locked);
    Config after;preserved&=after.load(garbage)&&after.bytes()==old.bytes();
    ok &= check(preserved,"failed atomic replacement preserves existing file");
    ok &= check(replacement.save(garbage)&&after.load(garbage)&&after.bytes()==replacement.bytes(),"atomic replacement writes the complete compatible record");
    replacement.rapid_key=0;
    ok &= check(replacement.save(garbage)&&after.load(garbage)&&after.rapid_key==0,"explicitly cleared rapid hotkey persists");
    std::error_code ignored;std::filesystem::remove(garbage,ignored);
    std::cout << "CONFIG " << output.string() << '\n';
    return ok;
}
}
