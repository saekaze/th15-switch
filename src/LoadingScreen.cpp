#include "LoadingScreen.hpp"
#include "ApplicationState.hpp"
#include "Platform.hpp"
#include "SdlCompat.hpp"
#include <algorithm>
namespace th15::sdl {
using namespace touhou::graphics;
void LoadingScreen::start(){
    // Script 17 at (960,784), as ApplicationState::loading_overlay makes it for the ending's loads.
    if(!overlay.start(app.animations,app.environment,2,17,{960,784,0}))host::log("loading animation unavailable: %s",overlay.error.c_str());
    active=true;clock=host::seconds();show();
}
void LoadingScreen::pump(){
    if(!active)return;const double now=host::seconds();u32 frames=u32((now-clock)*60);if(!frames)return;
    if(frames>4){frames=4;clock=now;}else clock+=frames/60.0;
    if(!overlay.advance(frames))host::log("loading animation stopped: %s",overlay.error.c_str());
    show();
}
void LoadingScreen::stop(){
    if(!active)return;active=false;overlay.stop();
    if(overlay.dropped)host::log("loading animation: %u sprites had no slot",overlay.dropped);
}
void LoadingScreen::show(){
    auto& g=app.graphics;auto& gpu=g.backend;const auto saved=gpu.state;
    gpu.state.target=GraphicsDevice::screen;gpu.state.depth=GraphicsDevice::depth;gpu.viewport({0,0,640,480,0,1});gpu.clear(1,0xff000000,1,0);
    auto& p=gpu.pipeline();p.depthTest=p.depthWrite=p.alphaTest=p.fog=p.textureTransform=false;p.blend=true;p.cull=Cull::None;p.colorMask=15;p.blendEquation=BlendEquation::Add;
    p.color.operation=p.alpha.operation=ColorOperation::Multiply;p.color.first=p.alpha.first={ArgumentSource::Texture};p.color.second=p.alpha.second={ArgumentSource::Diffuse};
    p.minFilter=p.magFilter=Filter::Linear;p.addressU=p.addressV=Address::Clamp;g.set_layout(VertexLayout::ScreenColorUv);
    const auto quad=[&](const float (&x)[4],const float (&y)[4],const float (&u)[4],const float (&v)[4],u32 color){
        AnmGeometryVertex data[4];for(u32 k=0;k<4;k++)data[k]={{x[k],y[k],0},1,color,{u[k],v[k]}};g.primitives(Topology::Strip,2,data,sizeof(AnmGeometryVertex));
    };
    // sig.anm: one picture (1280x960 in v1.00b) split over its textures from
    // left to right, each using its top-left pixel_width x pixel_height.
    if(const auto* sig=app.animations.resource(1)){
        float total=0,height=1e9f;for(const auto& t:sig->textures){total+=t.pixel_width;height=std::min<float>(height,t.pixel_height);}height=std::min(height,total*3/4);
        p.sourceBlend=BlendFactor::One;p.destinationBlend=BlendFactor::Zero;float left=0;
        if(total>0)for(u32 i=0;i<sig->textures.size();i++){const auto& t=sig->textures[i];if(!t.width||!t.height)continue;
            const float x0=left*640/total,x1=(left+t.pixel_width)*640/total,u1=float(t.pixel_width)/t.width,v1=height/t.height;left+=t.pixel_width;
            g.bind_texture(g.texture(*sig,i));quad({x0,x1,x0,x1},{0,0,480,480},{0,u1,0,u1},{0,0,v1,v1},0xffffffff);}
    }
    // The animation, lowest layer first; blend 1 is additive, anything else alpha.
    const u32 count=overlay.quads(quads.data(),u32(quads.size()),g);
    for(u32 i=0;i<count;i++){const auto& q=quads[i];
        p.sourceBlend=BlendFactor::SourceAlpha;p.destinationBlend=q.blend==1?BlendFactor::One:BlendFactor::InverseSourceAlpha;
        g.bind_texture(q.texture);quad(q.x,q.y,q.u,q.v,q.color);}
    gpu.present(GraphicsDevice::screen);gpu.state=saved;SDL_PumpEvents();
}
}
