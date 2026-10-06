// Touhou 15 for Nintendo Switch: native entry point.
//
// This replaces th15_web/cpp/sdl/BrowserRuntime.cpp. The ApplicationState it
// drives is upstream's (title, runs, endings, replays, checkpoints, audio and
// text); the browser parts become: files on the SD card instead of IDBFS, a
// vsync-paced 60 Hz loop instead of requestAnimationFrame, and Switch
// buttons / the touch screen feeding the same keyboard map, gamepad sampling
// and TouchController the web launcher forwards into.
#include "ApplicationState.hpp"
#include "Platform.hpp"
#include "SdlCompat.hpp"
#include "../game/GameKeyboard.hpp"
#include "../portable/sdl/FrameCadence.hpp"
#include "../portable/input/TouchController.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#ifdef __SWITCH__
#include <switch.h>
#endif

namespace th15::sdl {
namespace {
std::unique_ptr<ApplicationState> app;
struct Key {const char* code;const char* sdl;u32 scan,vk;bool hosted=false;SDL_Scancode native=SDL_SCANCODE_UNKNOWN;};
#include "../portable/input/KeyboardMap.inc"
touhou::input::TouchController gestures;
GameInput controls;
double window_start=0;u32 window_ticks=0;float measured_fps=60;
// Presentation counts committed frames, independently of simulation ticks
// and replay acceleration (upstream's on-screen FPS figure).
double presentation_start=-1;u32 presentation_frames=0;float presentation_fps=60;
FrameCallback fps_drawing;

i32 draw_frame_rate(void*){
    if(!app||app->title||app->ending||!app->run)return 1;
    // Original 4336e0 / 40c800: draw priority 72, small ASCII font, no shadow.
    HudTextStyle style;style.font=1;style.color=presentation_fps<30?0xff5050ff:presentation_fps<40?0xffa0a0ff:0xffffffff;
    char text[32];std::snprintf(text,sizeof text,"%2.1ffps",double(presentation_fps)+.05);
    return app->captions.enqueue({{588,470,0},style,text})?1:5;
}
Key* key(const char* code){for(auto& k:keyboard_map)if(!std::strcmp(k.code,code))return &k;return nullptr;}
void host_key(const char* code,bool down){if(auto* k=key(code))k->hosted=down;}
void clear_inputs(){
    for(auto& k:keyboard_map)k.hosted=false;gestures.reset();controls={};
    if(app){app->keyboard_state={};app->touch={};if(app->scene())app->scene()->battle.player->motion.touch={};}
}
touhou::input::TouchState touch_state(){
    touhou::input::TouchState value;if(!app||!app->scene()||!app->session)return value;
    if(app->pause&&app->pause->state.screen!=PauseScreen::Inactive)return value;
    if(app->progress.replay){value.context=3;return value;}
    auto& stage=*app->scene();if(stage.messages.active()){value.context=2;return value;}
    auto& player=*stage.battle.player;auto& motion=player.motion;
    value.context=1;value.instance=app->progress.stage;value.ready=player.life.state==1&&!(app->progress.scene_flags&(4|0x70|0x4000|0x10000));
    value.x=motion.position.x;value.y=motion.position.y;value.fast=float(motion.normal_speed)/128;value.slow=float(motion.focus_speed)/128;
    value.min_x=-184;value.max_x=184;value.min_y=32;value.max_y=432;return value;
}

// Switch controller as TH15's gamepad (upstream samples an SDL joystick the
// same way): Key Config binds shot / bomb / focus / pause to these button
// numbers, and the Switch defaults below give the layout of the other ports.
//   0 B  1 A  2 L/ZL  3 R/ZR  4 +  5 X  6 Y
// The d-pad and sticks only move - they are never buttons, so Key Config
// cannot pick them. R/ZR also holds Ctrl (skip dialogue) and X / Y keep their
// Enter / Retry shortcuts while no action is bound to them. − = snapshot.
constexpr i16 switch_defaults[4]{0,1,2,4}; // shot B, bomb A, focus L, pause +
struct PadInput {u32 buttons=0;float x=0,y=0;bool minus=false;};
#ifdef __SWITCH__
PadState pad;
PadInput read_pad(){
    padUpdate(&pad);PadInput p;if(!padIsConnected(&pad))return p;
    const u64 held=padGetButtons(&pad);const auto stick=padGetStickPos(&pad,0);
    const u64 map[7]{HidNpadButton_B,HidNpadButton_A,HidNpadButton_L|HidNpadButton_ZL,HidNpadButton_R|HidNpadButton_ZR,HidNpadButton_Plus,HidNpadButton_X,HidNpadButton_Y};
    for(u32 b=0;b<7;++b)if(held&map[b])p.buttons|=1u<<b;
    p.x=float(stick.x)/32768;p.y=-float(stick.y)/32768;
    if(held&HidNpadButton_Left)p.x=-1;if(held&HidNpadButton_Right)p.x=1;if(held&HidNpadButton_Up)p.y=-1;if(held&HidNpadButton_Down)p.y=1;
    p.minus=held&HidNpadButton_Minus;return p;
}
#else
PadInput read_pad(){return {};}
#endif
bool snapshot_held=false,snapshot_requested=false;

// Upstream sample_controller: bound buttons -> actions, half-way stick -> arrows.
u32 controller_keys(const PadInput& pad){
    u32 keys=0;constexpr u32 actions[]={1,2,8,0x100};
    for(u32 i=0;i<4;i++){const i32 binding=app->controller.values[i];if(binding>=0&&(pad.buttons&(1u<<(u32(binding)&31))))keys|=actions[i];}
    if(pad.x<-.5f)keys|=0x40;if(pad.x>.5f)keys|=0x80;if(pad.y<-.5f)keys|=0x10;if(pad.y>.5f)keys|=0x20;return keys;
}
bool bound(i32 button){for(u32 i=0;i<4;i++)if(app->controller.values[i]==button)return true;return false;}

void touch_event(const SDL_Event& e){
    // SDL2 reports touches normalised to the whole screen; the game wants the
    // 4:3 picture, which is pillarboxed inside it.
    const auto& p=app->graphics.backend.picture;if(p.width<=0||p.height<=0)return;
    const float x=(e.tfinger.x*p.drawable_width-p.x)/p.width,y=(e.tfinger.y*p.drawable_height-p.y)/p.height;
    const int type=e.type==SDL_FINGERDOWN?0:e.type==SDL_FINGERUP?2:1;
    if(type==0&&(x<0||x>1||y<0||y>1))return;
    gestures.pointer(type,int(e.tfinger.fingerId),std::clamp(x,0.f,1.f),std::clamp(y,0.f,1.f),SDL_GetTicks(),touch_state(),false);
}

bool sample_and_tick(){
    const PadInput pad=read_pad();
    host_key("ControlLeft",pad.buttons&(1u<<3));
    host_key("Enter",(pad.buttons&(1u<<5))&&!bound(5));host_key("KeyR",(pad.buttons&(1u<<6))&&!bound(6));
    if(pad.minus&&!snapshot_held)snapshot_requested=true;snapshot_held=pad.minus;
    bool keys[256]{};const auto* physical=SDL_GetKeyboardState(nullptr);
    for(const auto& k:keyboard_map)if(k.hosted||(k.native!=SDL_SCANCODE_UNKNOWN&&physical[k.native])){if(k.vk<256)keys[k.vk]=true;if(k.vk>=160&&k.vk<=165)keys[16+(k.vk-160)/2]=true;if(k.scan==28||k.scan==156)keys[13]=true;}
    app->keyboard_state.format=2;for(u32 i=0;i<256;i++)app->keyboard_state.keys[i]=keys[i]?128:0;
    const auto sample=gestures.sample(touch_state(),SDL_GetTicks(),keys[16],keys[37]||keys[38]||keys[39]||keys[40]);
    for(u32 i=0;i<256;i++)keys[i]=keys[i]||sample.keys[i];app->touch={sample.motion,sample.x,sample.y};
    const u32 held=keyboard_keys(keys)|controller_keys(pad);controls.update(held);app->controller_buttons=pad.buttons;
    app->numbered_chapter=0;for(u32 i=1;i<=9;i++)if(keys[48+i]){app->numbered_chapter=i;break;}
    const bool ok=app->step(controls.held,controls.pressed,controls.repeated,measured_fps,false);
    const double now=host::seconds();if(!window_start)window_start=now;
    if(ok){window_ticks++;const double duration=now-window_start;if(duration>=1){measured_fps=float(window_ticks/duration);window_ticks=0;window_start=now;}}
    return ok;
}

// Switch port: a simple bar while start-up bakes the text glyphs (first
// launch only; later launches reuse fontcache/) and reads th15.dat.
void loading_bar(float fraction){
    static double last=-1;const double now=host::seconds();if(fraction>0&&fraction<1&&now-last<1.0/30)return;last=now;
    auto& gpu=app->graphics.backend;gpu.state.target=GraphicsDevice::screen;gpu.state.depth=GraphicsDevice::depth;gpu.viewport({0,0,640,480,0,1});
    const i32 x0=160,x1=480,y0=400,y1=412,fill=x0+2+i32(float(x1-x0-4)*std::clamp(fraction,0.f,1.f));
    gpu.clear(1,0xff000000,1,0);const i32 frame[]{x0,y0,x1,y1};gpu.clear(1,0xff606060,1,0,frame,1);
    const i32 inner[]{x0+2,y0+2,x1-2,y1-2};gpu.clear(1,0xff000000,1,0,inner,1);
    if(fill>x0+2){const i32 bar[]{x0+2,y0+2,fill,y1-2};gpu.clear(1,0xffe0e0e0,1,0,bar,1);}
    gpu.present(GraphicsDevice::screen);SDL_PumpEvents();
}

// Switch port: '−' saves the current frame as a BMP into snapshot/ next to
// the saves, like the PC game's snapshot folder.
bool snapshot(){
    auto& gpu=app->graphics.backend;gpu.flush();gpu.read(GraphicsDevice::screen);auto* image=app->graphics.pixels(GraphicsDevice::screen);if(!image||image->pixels.empty())return false;
    const auto dir=host::paths().save_file("snapshot");host::make_directory(dir);char file[64];std::string path;
    for(u32 n=0;n<1000;++n){std::snprintf(file,sizeof(file),"/th15_%03u.bmp",n);path=dir+file;if(!host::exists(path))break;}
    const u32 w=image->width,h=image->height,row=w*3,pad=(4-row%4)%4,size=54+(row+pad)*h;std::vector<u8> bmp(size,0);
    auto put=[&](u32 at,u32 v,u32 n){for(u32 i=0;i<n;++i)bmp[at+i]=u8(v>>(8*i));};
    bmp[0]='B';bmp[1]='M';put(2,size,4);put(10,54,4);put(14,40,4);put(18,w,4);put(22,h,4);put(26,1,2);put(28,24,2);put(34,size-54,4);
    for(u32 y=0;y<h;++y){const u8* src=image->pixels.data()+(h-1-y)*image->pitch;u8* dst=bmp.data()+54+y*(row+pad);for(u32 x=0;x<w;++x){dst[x*3]=src[x*4];dst[x*3+1]=src[x*4+1];dst[x*3+2]=src[x*4+2];}}
    const bool ok=host::write_file(path,bmp);host::log("snapshot %s: %s",path.c_str(),ok?"ok":"failed");return ok;
}

void fatal(const std::string& message){
    host::log("fatal: %s",message.c_str());
    // Tear down the game (GL context, window, audio) so the console can take
    // the framebuffer, then wait for +.
    app.reset();SDL_Quit();
#ifdef __SWITCH__
    consoleInit(nullptr);
    std::printf("\n  Touhou 15: Legacy of Lunatic Kingdom - Switch port\n\n  Error: %s\n\n",message.c_str());
    std::printf("  Data folder: %s\n  Needed: th15.dat (v1.00b) and msgothic.ttc. Optional: thbgm.dat\n\n  Press + to exit.\n",host::paths().data.c_str());
    consoleUpdate(nullptr);
    while(appletMainLoop()){padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;consoleUpdate(nullptr);}
    consoleExit(nullptr);
#else
    std::fprintf(stderr,"%s\n",message.c_str());
#endif
}

bool start(){
    app=std::make_unique<ApplicationState>();
    if(!app->graphics.initialize()){app->failure=app->graphics.error;return false;}
    loading_bar(0);double begin=host::seconds();
    app->fonts.progress=[](float f){loading_bar(.9f*f);};const bool fonts_ok=app->fonts.initialize();app->fonts.progress=nullptr;
    if(!fonts_ok){app->failure=app->fonts.error;return false;}
    loading_bar(.9f);host::log("fonts ready in %.1fs",host::seconds()-begin);begin=host::seconds();
    if(!app->initialize(true))return false;
    host::log("game initialised in %.1fs",host::seconds()-begin);
    // First launch (no th15.cfg yet) leaves every action on button 0; use the
    // Switch layout instead. Key Config changes are kept afterwards.
    auto& v=app->controller.values;
    if(v[0]==v[1]&&v[1]==v[2]&&v[2]==v[3]){for(u32 i=0;i<4;i++)v[i]=switch_defaults[i];if(!app->save(app->controller))return false;}
    fps_drawing.owner=nullptr;fps_drawing.enabled=true;fps_drawing.run=draw_frame_rate;
    if(app->scheduler.add(fps_drawing,FramePass::Draw,72)<0)return app->fail("Frame-rate draw callback registration failed");
    for(auto& k:keyboard_map)k.native=SDL_GetScancodeFromName(k.sdl);
    clear_inputs();return true;
}
}
}

int main(int argc,char** argv){
    using namespace th15::sdl;namespace host=th15::host;using th15::u32;
    host::locate_data(argc,argv);
#ifdef __SWITCH__
    // Same CPU boost the other native ports use; the GPU stays with the applet.
    if(R_SUCCEEDED(clkrstInitialize())){ClkrstSession cpu;if(R_SUCCEEDED(clkrstOpenSession(&cpu,PcvModuleId_CpuBus,3))){clkrstSetClockRate(&cpu,1785000000);clkrstCloseSession(&cpu);}clkrstExit();}
    padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&pad);
    appletSetFocusHandlingMode(AppletFocusHandlingMode_SuspendHomeSleep);
#endif
    host::log("th15-switch: data %s",host::paths().data.c_str());
    if(!host::exists(host::paths().data_file("th15.dat"))){fatal("th15.dat not found");return 1;}
    if(!start()){fatal(app?app->failure:"start-up failed");return 1;}
    // Vsync paces the loop at the panel's 60 Hz: one game tick per swap.
    // Measured time only adds catch-up ticks after a real stall (bounded to
    // 4 like upstream FrameCadence), so vsync jitter never drops a frame.
    constexpr double interval=touhou::sdl::FrameCadence::interval;
    double debt=0,previous=host::seconds();bool running=true,ok=true;
    presentation_start=previous;
    while(running&&ok
#ifdef __SWITCH__
          &&appletMainLoop()
#endif
    ){
        SDL_Event e;while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT)running=false;
            else if(e.type==SDL_FINGERDOWN||e.type==SDL_FINGERUP||e.type==SDL_FINGERMOTION)touch_event(e);
            else if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_LOST)clear_inputs();
        }
        const double now=host::seconds();debt+=std::clamp(now-previous,0.,.1);previous=now;
        u32 ticks=u32(std::floor((debt+interval*.5)/interval));ticks=std::min(4u,std::max(ticks,1u));
        debt=std::clamp(debt-ticks*interval,-interval*.5,.1);
        app->graphics.backend.defer=true;u32 completed=0;
        for(u32 n=0;n<ticks&&ok;++n){ok=sample_and_tick();++completed;if(app->exiting)break;}
        if(ok&&snapshot_requested){snapshot_requested=false;snapshot();}
        app->graphics.backend.commit();app->graphics.backend.defer=false;app->audio_device.pump();
        if(ok&&completed)++presentation_frames;
        const double elapsed=host::seconds()-presentation_start;if(elapsed>=1){presentation_fps=float(presentation_frames/elapsed);presentation_start=host::seconds();presentation_frames=0;}
        if(ok&&app->exiting)running=false; // title menu Quit
    }
    if(!ok){fatal(app->failure.empty()?"game stopped":app->failure);return 1;}
    app->save_settings();
    app.reset();
    SDL_Quit();
    return 0;
}
