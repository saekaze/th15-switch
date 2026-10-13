// Switch port, from シエル's TH15 PSP port (shared for this port; see
// https://github.com/kan8223-dotcom/th10-psp-native): the
// original's loading animation (ascii.anm script 17 at 960,784, as the
// ending's loading_overlay creates it: "NowLoading..." sliding in, 少女祈祷中
// pulsing, a moon let go every 4 frames) for the start-up load, which has no
// frame of its own here.
// The game's VM runs the scripts and the game's transforms place the quads,
// but on its own: its own slots instead of the registry, its own random source
// and a copy of the environment, so nothing the game computes changes.
#pragma once
#include "../game/AnmManager.hpp"
#include "LoadingQuad.hpp"
#include <array>
#include <string>
namespace th15::sdl {
class GraphicsDevice;
class LoadingOverlay final:public AnmObjectHost {
    // A fixed set of VMs (the moons live 120 frames, one every 4: about 33 at
    // once), so a load is not cut into by small blocks coming and going.
    static constexpr u32 capacity=48;
    struct Slot {AnmVm vm;u32 order=0;bool used=false;};
    AnmManager* manager=nullptr;i32 bank=-1;Rng random{};AnmEnvironment environment{};
    std::array<Slot,capacity> slots;AnmVm discard;u32 next_order=0;bool running=false;
    AnmVm* adopt(const AnmVm& source);   // a bank template, set up as AnmManager::bind_template leaves it
public:
    std::string error;u32 dropped=0;   // children not made for want of a slot
    bool start(AnmManager& animations,const AnmEnvironment& game,i32 bank,i32 script,const Vec3& at);
    void stop()noexcept;
    bool active()const noexcept{return running;}
    bool advance(u32 frames);   // 60 Hz frames
    u32 quads(LoadingQuad* out,u32 room,GraphicsDevice& graphics);   // visible sprites, lowest layer first
    AnmVm* spawn_child(AnmVm& source,i32 script,u32 ordering)override;
    AnmVm* spawn_detached(AnmVm& source,i32 script)override;
    bool spawn_effect(AnmVm&,i32)override{return true;}
    bool release_tree(AnmVm&)override{return true;}
};
}
