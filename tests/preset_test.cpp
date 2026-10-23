#include "PresetManager.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream file(argv[1], std::ios::binary);
    assert(file.good());
    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), {});
    talclone::PresetManager manager;
    assert(bytes.size() == talclone::PresetManager::recordSize * 120);
    assert(manager.importChunk(bytes.data(), bytes.size(), 0));
    // Check every decoded record against the separately generated factory table.
    for (int i = 0; i < 120; ++i) {
        const auto& preset = talclone::factoryPresets[static_cast<std::size_t>(i)];
        assert(manager.name(i) == preset.name);
        assert(manager.values(i) == preset.parameters);
        assert(manager.midiMap(i) == preset.midiMap);
    }
    assert(manager.name(0) == "Startup");
    assert(manager.values(4)[28] == 9.0f); // Raw dropdown indices must survive.
    assert(!manager.importChunk(bytes.data(), bytes.size() - 1, 0));
    assert(!manager.importChunk(nullptr, bytes.size(), 0));
    auto invalid = bytes;
    invalid[24] = 0; invalid[25] = 0; invalid[26] = 0xc0; invalid[27] = 0x7f; // NaN
    manager.rename(0, "Keep on failure");
    assert(!manager.importChunk(invalid.data(), invalid.size(), 0));
    assert(manager.name(0) == "Keep on failure");
    assert(manager.importChunk(bytes.data(), talclone::PresetManager::recordSize, 7));
    assert(manager.name(7) == "Startup");
    assert(manager.values(7) == talclone::factoryPresets[0].parameters);
    std::cout << "120 presets: names, parameters, MIDI maps and atomic rejection passed\n";
}
