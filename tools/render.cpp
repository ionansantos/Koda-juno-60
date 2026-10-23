#include "DSP/SynthDSP.h"
#include "FactoryPresets.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

void u32(std::ostream& out, std::uint32_t value) {
    for (int n = 0; n < 4; ++n) out.put(static_cast<char>((value >> (8 * n)) & 255));
}
void u16(std::ostream& out, std::uint16_t value) {
    out.put(static_cast<char>(value & 255)); out.put(static_cast<char>(value >> 8));
}
void sample(std::ostream& out, float value) {
    std::uint32_t bits; std::memcpy(&bits, &value, 4); u32(out, bits);
}
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: render output.wav preset_index_0_to_119\n"; return 2; }
    try {
        const int program = std::stoi(argv[2]);
        if (program < 0 || program >= 120) throw std::out_of_range("Preset index");
        constexpr int rate = 44100, length = rate * 4;
        std::ofstream out(argv[1], std::ios::binary);
        if (!out) throw std::runtime_error("Cannot write output");
        out.write("RIFF", 4); u32(out, 48 + length * 8); out.write("WAVEfmt ", 8);
        u32(out, 16); u16(out, 3); u16(out, 2); u32(out, rate);
        u32(out, rate * 8); u16(out, 8); u16(out, 32);
        out.write("fact", 4); u32(out, 4); u32(out, length);
        out.write("data", 4); u32(out, length * 8);
        talclone::SynthDSP synth; synth.setSampleRate(rate);
        synth.setParameters(talclone::factoryPresets[static_cast<std::size_t>(program)].parameters);
        synth.setTempo(120); synth.noteOn(69, 100.f / 127.f);
        // Fixed MIDI fixture: A4, velocity100, noteOff at1s; retain native preset transpose.
        for (int i = 0; i < length; ++i) {
            if (i == rate) synth.noteOff(69);
            float left = 0, right = 0; synth.processStereo(left, right);
            sample(out, left); sample(out, right);
        }
        std::cout << "Rendered " << talclone::factoryPresets[static_cast<std::size_t>(program)].name
                  << ", stereo float32, 44100Hz, 4s\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
