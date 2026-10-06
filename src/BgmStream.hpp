// Switch host: a thbgm.dat track as a miniaudio data source.
#pragma once
#include "MiniaudioConfig.hpp"
#include "../game/Types.hpp"
#include <cstdio>
#include <vector>
namespace th15::host {
// One open thbgm.dat shared by every cached track; `position` is where the
// FILE currently is, so a track only seeks when another one moved it.
struct BgmFile {FILE* file=nullptr;u64 position=~0ull;};
struct PcmStream {
    ma_data_source_base base{};BgmFile* source=nullptr;u64 offset=0,frames=0,cursor=0;u32 channels=2,rate=44100;
    std::vector<i16> scratch;
};
extern const ma_data_source_vtable pcm_vtable;
}
