#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// Small generated sounds so feedback never depends on the Windows error beep.
// The buffers are static and live for the process; PlaySound reads them
// asynchronously with SND_MEMORY.
namespace sound {
struct Note { double frequency; int milliseconds; };

inline void append16(std::vector<BYTE>& out, std::uint16_t value) {
    out.push_back(BYTE(value & 0xff));
    out.push_back(BYTE((value >> 8) & 0xff));
}
inline void append32(std::vector<BYTE>& out, std::uint32_t value) {
    out.push_back(BYTE(value & 0xff));
    out.push_back(BYTE((value >> 8) & 0xff));
    out.push_back(BYTE((value >> 16) & 0xff));
    out.push_back(BYTE((value >> 24) & 0xff));
}

inline std::vector<BYTE> makeWav(const std::vector<Note>& notes) {
    constexpr int rate = 44100;
    constexpr double volume = 0.16;
    int totalSamples = 0;
    for (const auto& note : notes) totalSamples += std::max(1, note.milliseconds) * rate / 1000;

    std::vector<BYTE> out;
    out.reserve(std::size_t(44 + totalSamples * 2));
    out.insert(out.end(), {'R', 'I', 'F', 'F'});
    append32(out, std::uint32_t(36 + totalSamples * 2));
    out.insert(out.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    append32(out, 16);
    append16(out, 1);
    append16(out, 1);
    append32(out, rate);
    append32(out, rate * 2);
    append16(out, 2);
    append16(out, 16);
    out.insert(out.end(), {'d', 'a', 't', 'a'});
    append32(out, std::uint32_t(totalSamples * 2));

    for (const auto& note : notes) {
        const int samples = std::max(1, note.milliseconds) * rate / 1000;
        const int fade = std::min(samples / 3, rate / 100);
        double phase = 0;
        for (int i = 0; i < samples; ++i) {
            double envelope = 1;
            if (i < fade) envelope = double(i) / fade;
            if (i >= samples - fade) envelope = std::min(envelope, double(samples - 1 - i) / fade);
            const double value = note.frequency > 0
                ? std::sin(phase) * volume * envelope * 32767.0
                : 0.0;
            phase += 2.0 * 3.14159265358979323846 * note.frequency / rate;
            const auto sample = std::int16_t(std::clamp(value, -32767.0, 32767.0));
            out.push_back(BYTE(std::uint16_t(sample) & 0xff));
            out.push_back(BYTE((std::uint16_t(sample) >> 8) & 0xff));
        }
    }
    return out;
}

inline void play(const std::vector<BYTE>& wav) {
    if (wav.size() <= 44) return;
    PlaySoundW(reinterpret_cast<LPCWSTR>(wav.data()), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

// A soft descending "mm-mm" used when leaving with unsaved changes.
inline void playUnsaved() {
    static const auto no = makeWav({{392.00, 90}, {329.63, 150}});
    play(no);
}
} // namespace sound
