#pragma once
#include <array>
#include <cstdint>
#include <vector>
namespace talclone {
// Network reconstructed from retained REA initializer and processBlock evidence.
// Deterministic modulation seeds intentionally replace the reference's ambient rand().
class StereoReverb {
public:
    struct Settings { float wet=0, size=.8f, delay=0, tone=.5f; };
    void prepare(double sampleRate);
    void reset() noexcept;
    void setParameters(Settings) noexcept;
    void process(float& left, float& right) noexcept;
private:
    struct Filter {
        double b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0,previous=0;
        void configure(double rate,double frequency,bool highpass) noexcept;
        float tick(float) noexcept;
        void clear() noexcept { z1=z2=previous=0; }
    };
    struct Branch {
        std::vector<float> buffer;
        std::size_t position=0;
        std::uint32_t seed=1, initialSeed=1;
        float ramp=0, step=0, initialStep=0, period=1, damp=0, coefficient=0;
        float tick(float input,float size,float sign) noexcept;
        void clear() noexcept;
    };
    struct Allpass {
        std::vector<float> buffer;
        std::size_t position=0;
        float tick(float) noexcept;
    };
    double sampleRate_=44100;
    Settings settings_;
    bool configured_=false, clearPending_=false;
    float dry_=1, wet_=0, size_=0;
    std::vector<float> predelay_;
    std::size_t position_=0, tap_=1;
    Filter highpass_,lowpass_;
    std::array<std::array<Branch,6>,2> branches_;
    std::array<std::array<Allpass,11>,2> allpasses_;
};
}
