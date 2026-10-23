#pragma once
#include <array>
#include <atomic>
#include <cstddef>
namespace talclone {
// One owner for current and per-program controller tables. No strings or allocation on the audio path.
class MidiMappings {
public:
 static constexpr int controllers=128,programs=120;
 MidiMappings() noexcept {clear();for(auto& bank:banks)for(auto& value:bank)value.store(-1);}
 int target(int cc) const noexcept {return cc>=0&&cc<controllers?current[static_cast<std::size_t>(cc)].load():-1;}
 void assign(int cc,int index) noexcept {if(cc>=0&&cc<controllers)current[static_cast<std::size_t>(cc)].store(index);}
 void clear() noexcept {for(auto& value:current)value.store(-1);}
 void capture(int program) noexcept {if(program>=0&&program<programs)for(int cc=0;cc<controllers;++cc)banks[static_cast<std::size_t>(program)][static_cast<std::size_t>(cc)].store(target(cc));}
 void select(int program,bool locked) noexcept {if(!locked&&program>=0&&program<programs)for(int cc=0;cc<controllers;++cc)assign(cc,banks[static_cast<std::size_t>(program)][static_cast<std::size_t>(cc)].load());}
 void setSaved(int program,int cc,int index) noexcept {if(program>=0&&program<programs&&cc>=0&&cc<controllers)banks[static_cast<std::size_t>(program)][static_cast<std::size_t>(cc)].store(index);}
 int saved(int program,int cc) const noexcept {return banks[static_cast<std::size_t>(program)][static_cast<std::size_t>(cc)].load();}
private:
 std::array<std::atomic<int>,controllers> current{};
 std::array<std::array<std::atomic<int>,controllers>,programs> banks{};
};
}
