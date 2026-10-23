#pragma once
#include <array>
#include <cstddef>
#include <vector>

namespace talclone {
// Reconstructed from retained TAL 5.1.5 REA evidence 020, 024, 025 and 031.
class StereoDelay {
public:
    struct Settings { float wet=0, time=.5f, spread=.5f, tone=.5f, feedback=0; bool sync=false; };
    void prepare(double sampleRate);
    void reset() noexcept;
    void setParameters(Settings, double bpm=120) noexcept;
    void process(float& left, float& right) noexcept;
    static double delaySeconds(float time, bool sync, double bpm) noexcept;
    static const std::array<const char*,21> divisions;
private:
    struct Channel {
        std::vector<float> ring;
        std::size_t write=0;
        float target=.01f, smoothed=0, feedback=0;
        float lpState=0, lpHalf=0, lpOutput=0, hpInput=0, hpOutput=0;
    };
    float tick(Channel&, float) noexcept;
    std::array<Channel,2> channels;
    double rate=44100;
    float smoothing=2000, smoothingScale=1.f/2001.f, lowpass=0, highpass=.999f, wet=0, dry=1;
    Settings settings;
    double tempo=120;
};
}
