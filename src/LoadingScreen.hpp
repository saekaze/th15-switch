// Switch port: the original start-up screen while TH15 loads - sig.anm's
// picture ("東方Project / 15th Project Shrine Maiden ...") with ascii.anm's
// 少女祈祷中 animation (LoadingOverlay) on top - instead of a plain bar. All of
// it comes from the player's th15.dat. The load blocks the main loop, so the
// loader calls pump() between its steps (each texture, the glyph bake's
// progress) and a frame is drawn whenever one is due at 60 Hz.
#pragma once
#include "LoadingOverlay.hpp"
#include <array>
namespace th15::sdl {
struct ApplicationState;
class LoadingScreen {
    ApplicationState& app;LoadingOverlay overlay;std::array<LoadingQuad,64> quads{};double clock=0;bool active=false;
    void show();
public:
    explicit LoadingScreen(ApplicationState& state):app(state){}
    void start();   // sig.anm (bank 1) and ascii.anm (bank 2) are loaded
    void pump();    // between loading steps
    void stop();
};
}
