// Switch port (from the TH15 PSP port): one sprite quad of the loading
// animation (LoadingOverlay), drawn by LoadingScreen.
#pragma once
#include <cstdint>
namespace th15::sdl {
// In the game's screen space (640 x 480), corners in AnmRenderer's order (top
// left, top right, bottom left, bottom right); UVs in the declared texture size;
// color is the VM's ARGB; blend is the VM's blend mode (0 alpha, 1 additive).
struct LoadingQuad {float x[4],y[4],u[4],v[4];std::uint32_t color=0,blend=0,texture=0;std::int32_t layer=0;};
}
