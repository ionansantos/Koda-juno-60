#pragma once
#include "FactoryPresets.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace talclone {
class PresetManager {
public:
    static constexpr int count = 120;
    static constexpr std::size_t recordSize = 0x4d0;
    PresetManager() {
        for (int i = 0; i < count; ++i) {
            names_[static_cast<std::size_t>(i)] = factoryPresets[static_cast<std::size_t>(i)].name;
            values_[static_cast<std::size_t>(i)] = factoryPresets[static_cast<std::size_t>(i)].parameters;
            maps_[static_cast<std::size_t>(i)] = factoryPresets[static_cast<std::size_t>(i)].midiMap;
        }
    }
    const std::string& name(int i) const { return names_.at(static_cast<std::size_t>(i)); }
    const std::array<float, 47>& values(int i) const { return values_.at(static_cast<std::size_t>(i)); }
    const std::array<int, 255>& midiMap(int i) const { return maps_.at(static_cast<std::size_t>(i)); }
    void rename(int i, std::string name) { names_.at(static_cast<std::size_t>(i)) = std::move(name); }

    // Verified i386 bank/preset payload, not an .fxb container. Decode explicit
    // little endian; fail atomically on wrong size or NaN. Two factory names
    // occupy all 24 bytes; bound those names without reading parameter bytes.
    bool importChunk(const void* data, std::size_t size, int current) {
        if (data == nullptr || current < 0 || current >= count ||
            (size != recordSize && size != recordSize * count)) return false;
        auto candidate = *this;
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        const int records = size == recordSize ? 1 : count;
        for (int n = 0; n < records; ++n) {
            const auto i = static_cast<std::size_t>(records == 1 ? current : n);
            const auto* record = bytes + recordSize * static_cast<std::size_t>(n);
            const void* end = std::memchr(record, 0, 24);
            candidate.names_[i].assign(reinterpret_cast<const char*>(record),
                end == nullptr ? 24 : static_cast<std::size_t>(static_cast<const std::uint8_t*>(end) - record));
            for (std::size_t p = 0; p < 47; ++p) {
                const auto bits = read32(record + 24 + p * 4);
                float value;
                std::memcpy(&value, &bits, sizeof value);
                if (!std::isfinite(value)) return false;
                candidate.values_[i][p] = value;
            }
            for (std::size_t cc = 0; cc < 255; ++cc)
                candidate.maps_[i][cc] = static_cast<std::int32_t>(read32(record + 0xd4 + cc * 4));
        }
        *this = std::move(candidate);
        return true;
    }
private:
    static std::uint32_t read32(const std::uint8_t* p) {
        return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
               (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
    }
    std::array<std::string, count> names_;
    std::array<std::array<float, 47>, count> values_{};
    std::array<std::array<int, 255>, count> maps_{};
};
} // namespace talclone
