#pragma once
#include "GraphicsDevice.hpp"
#include "../game/TextLayout.hpp"
#include <functional>
namespace th15::sdl {
class FontDevice {GraphicsDevice& graphics;TextRaster raster;std::array<bool,15> loaded{};bool load(u32 slot);
public:
    std::string error;u32 writes=0;bool fallback=false;
    std::function<void(float)> progress; // Switch port: first-launch bake progress
    explicit FontDevice(GraphicsDevice& g):graphics(g){}
    bool initialize();bool preload(i32 style);bool text(AnmVm&,const DialogueText&);
};
}
