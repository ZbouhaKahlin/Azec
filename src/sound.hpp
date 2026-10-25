#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

// Interaction sounds synthesized live, ported from the cuelume library
// (github.com/Danilaa1/cuelume, MIT) that monocode uses: every cue is a small
// recipe of tone and filtered-noise layers with exponential envelopes and an
// optional shimmer echo, rendered offline into a WAV buffer. The buffers are
// static and live for the process; PlaySound reads them asynchronously with
// SND_MEMORY, so feedback never depends on the Windows error beep.
namespace sound {
inline bool dryRun=false;
inline void setDryRun(bool enabled){dryRun=enabled;}
constexpr double kVolume=0.55;      // monocode's "quiet enough to run in the background"
constexpr double kOutputGain=4.0;   // cuelume's shared output stage

struct ToneLayer{const char* waveform;double frequency;double glideTo;double glideTime;double detune;double offset,attack,decay,peak;};
struct NoiseLayer{const char* filter;double frequency,q;double offset,attack,decay,peak;};
struct Shimmer{double delay,feedback,wet,lowpass;};
struct Recipe{double masterGain;std::vector<ToneLayer> tones;std::vector<NoiseLayer> noises;std::optional<Shimmer> shimmer;};

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

// Second-order sections matching the Web Audio biquad filters (RBJ cookbook).
struct Biquad {
    double b0=1,b1=0,b2=0,a1=0,a2=0,x1=0,x2=0,y1=0,y2=0;
    Biquad(const char* type,double frequency,double q,int rate){
        const double omega=2.0*3.14159265358979323846*frequency/rate,cosine=std::cos(omega),alpha=std::sin(omega)/(2.0*q);
        if(std::strcmp(type,"lowpass")==0){b0=(1-cosine)/2;b1=1-cosine;b2=b0;}
        else{b0=alpha;b1=0;b2=-alpha;} // bandpass, constant 0 dB peak gain
        const double a0=1+alpha;a1=-2*cosine/a0;a2=(1-alpha)/a0;b0/=a0;b1/=a0;b2/=a0;
    }
    double process(double x){const double y=b0*x+b1*x1+b2*x2-a1*y1-a2*y2;x2=x1;x1=x;y2=y1;y1=y;return y;}
};

inline double envelope(double local,double attack,double decay,double peak){
    // Web Audio exponential ramps: 0.0001 -> peak across the attack, then
    // peak -> 0.0001 across the decay, sampled on the same curve.
    if(local<attack)return 0.0001*std::pow(peak/0.0001,local/attack);
    const double tail=local-attack;
    return peak*std::pow(0.0001/peak,std::min(1.0,tail/decay));
}

inline std::vector<BYTE> makeWav(const Recipe& recipe) {
    constexpr int rate = 44100;
    double seconds=0;
    for(const auto& layer:recipe.tones)seconds=std::max(seconds,layer.offset+layer.attack+layer.decay);
    for(const auto& layer:recipe.noises)seconds=std::max(seconds,layer.offset+layer.attack+layer.decay);
    double tail=0;
    if(recipe.shimmer&&recipe.shimmer->feedback>0&&recipe.shimmer->feedback<1)
        tail=recipe.shimmer->delay*(1+std::ceil(std::log(0.001)/std::log(recipe.shimmer->feedback)));
    else if(recipe.shimmer)tail=recipe.shimmer->delay;
    const int totalSamples=int((seconds+tail+0.05)*rate)+1;

    std::vector<double> mix(std::size_t(totalSamples),0.0);
    auto seed=std::uint32_t(0x9E3779B9u);
    const auto random=[&]{seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return double(seed)/4294967296.0*2.0-1.0;};
    for(const auto& layer:recipe.tones){
        const double frequency=layer.frequency*std::pow(2.0,layer.detune/1200.0);
        const int start=int(layer.offset*rate),samples=int((layer.attack+layer.decay)*rate);
        const bool glide=layer.glideTo>0;
        const double glideSamples=std::max(1.0,(layer.glideTime>0?layer.glideTime:layer.attack+layer.decay)*rate);
        double phase=0;
        for(int i=0;i<samples;++i){
            const double local=double(i)/rate;
            double f=frequency;
            if(glide)f=i<glideSamples?std::exp(std::log(frequency)+(std::log(layer.glideTo)-std::log(frequency))*(i/glideSamples)):layer.glideTo;
            phase+=2.0*3.14159265358979323846*f/rate;
            const double wave=std::strcmp(layer.waveform,"triangle")==0
                ?2.0*std::asin(std::sin(phase))/3.14159265358979323846
                :std::sin(phase);
            mix[std::size_t(start+i)]+=wave*envelope(local,layer.attack,layer.decay,layer.peak);
        }
    }
    for(const auto& layer:recipe.noises){
        const int start=int(layer.offset*rate),samples=int((layer.attack+layer.decay)*rate);
        Biquad filter(layer.filter,layer.frequency,layer.q,rate);
        for(int i=0;i<samples;++i)
            mix[std::size_t(start+i)]+=filter.process(random())*envelope(double(i)/rate,layer.attack,layer.decay,layer.peak);
    }
    if(recipe.shimmer){
        const auto& shimmer=*recipe.shimmer;
        const int delaySamples=std::max(1,int(shimmer.delay*rate));
        std::vector<double> echo(mix.size(),0.0);
        Biquad loop("lowpass",shimmer.lowpass,0.7,rate);
        for(std::size_t i=std::size_t(delaySamples);i<mix.size();++i)
            echo[i]=loop.process(mix[i-delaySamples]+shimmer.feedback*echo[i-delaySamples]);
        for(std::size_t i=0;i<mix.size();++i)mix[i]+=shimmer.wet*echo[i];
    }

    std::vector<BYTE> out;
    out.reserve(44+mix.size()*2);
    out.insert(out.end(),{'R','I','F','F'});
    append32(out, std::uint32_t(36+mix.size()*2));
    out.insert(out.end(),{'W','A','V','E','f','m','t',' '});
    append32(out, 16);
    append16(out, 1);
    append16(out, 1);
    append32(out, rate);
    append32(out, rate * 2);
    append16(out, 2);
    append16(out, 16);
    out.insert(out.end(),{'d','a','t','a'});
    append32(out, std::uint32_t(mix.size()*2));
    const double gain=recipe.masterGain*kVolume*kOutputGain;
    for(const double sample:mix){
        const auto value=std::int16_t(std::clamp(sample*gain,-1.0,1.0)*32767.0);
        out.push_back(BYTE(std::uint16_t(value) & 0xff));
        out.push_back(BYTE((std::uint16_t(value) >> 8) & 0xff));
    }
    return out;
}

inline void play(const std::vector<BYTE>& wav) {
    if (dryRun || wav.size() <= 44) return;
    PlaySoundW(reinterpret_cast<LPCWSTR>(wav.data()), nullptr,
               SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
}

// A quick mechanical click-clack for switches and output toggles.
inline void playToggle() {
    static const Recipe recipe{0.4,{},{{"bandpass",2200,1.6,0,0.001,0.016,0.12},{"bandpass",3800,1.6,0.024,0.001,0.02,0.1}},{}};
    static const auto wav=makeWav(recipe);play(wav);
}
// A short, rounded tap for Settings, without a pitch sweep or noise layer.
inline void playSettings() {
    static const Recipe recipe{0.35,{{"sine",392,0,0,0,0,0.004,0.055,0.055},{"sine",784,0,0,0,0,0.003,0.02,0.006}},{},{}};
    static const auto wav=makeWav(recipe);play(wav);
}
// A fast three-step locator tone for copy actions and small confirmations.
inline void playCopy() {
    static const Recipe recipe{0.4,{{"sine",740,0,0,0,0,0.002,0.055,0.05},{"sine",1110,0,0,0,0.045,0.002,0.055,0.045},{"sine",1665,0,0,0,0.09,0.002,0.07,0.04}},{},Shimmer{0.065,0.16,0.1,4200}};
    static const auto wav=makeWav(recipe);play(wav);
}
// A short, warm three-note ascending confirmation — "done", not a fanfare.
inline void playSuccess() {
    static const Recipe recipe{0.5,{{"sine",880,0,0,0,0,0.004,0.09,0.06},{"sine",1108.73,0,0,0,0.06,0.004,0.1,0.06},{"sine",1318.51,0,0,0,0.12,0.004,0.18,0.07}},{},Shimmer{0.1,0.22,0.16,4500}};
    static const auto wav=makeWav(recipe);play(wav);
}
// A rising harmonic portal with a soft tail — for arrivals and updates.
inline void playArrival() {
    static const Recipe recipe{0.44,{{"sine",220,440,0.32,0,0,0.04,0.34,0.055},{"sine",659.25,0,0,0,0.12,0.045,0.32,0.04},{"sine",987.77,0,0,0,0.19,0.045,0.34,0.032}},{{"lowpass",900,0.8,0,0.05,0.24,0.035}},Shimmer{0.16,0.28,0.18,3200}};
    static const auto wav=makeWav(recipe);play(wav);
}
// A muted knock followed by two descending tones — a calm, recoverable refusal.
inline void playUnsaved() {
    static const Recipe recipe{0.42,{{"triangle",440,0,0,0,0.025,0.004,0.09,0.045},{"triangle",349.23,0,0,0,0.1,0.004,0.14,0.04}},{{"bandpass",850,1.1,0,0.001,0.035,0.13}},{}};
    static const auto wav=makeWav(recipe);play(wav);
}
} // namespace sound
