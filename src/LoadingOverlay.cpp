// Switch port (from the TH15 PSP port): see LoadingOverlay.hpp.
#include "LoadingOverlay.hpp"
#include "../game/AnmCoordinates.hpp"
#include "../game/AnmTransforms.hpp"
#include "GraphicsDevice.hpp"
#include <algorithm>
namespace th15::sdl {
AnmVm* LoadingOverlay::adopt(const AnmVm& source){
    Slot* slot=nullptr;for(auto& s:slots)if(!s.used){slot=&s;break;}
    AnmVm& vm=slot?slot->vm:discard;if(slot){slot->used=true;slot->order=next_order++;}else ++dropped;   // no slot: made, never run or drawn
    // AnmManager::bind_template: the template, no parents, timers from zero.
    vm=source;vm.visual.translation={};vm.rotation_parent=vm.creation_parent=nullptr;vm.slowdown=0;vm.pending_interrupt=0;vm.reset_runtime_timers();
    vm.environment=&environment;vm.object_host=this;return &vm;
}
void LoadingOverlay::stop()noexcept{for(auto& s:slots){if(s.used)s.vm=AnmVm{};s.used=false;}running=false;}
bool LoadingOverlay::start(AnmManager& animations,const AnmEnvironment& game,i32 id,i32 script,const Vec3& at){
    stop();error.clear();manager=&animations;bank=id;environment=game;random=Rng{};random.seed=0x1500;
    const AnmVm* source=script>=0?animations.script_template(id,u32(script)):nullptr;if(!source){error="Loading animation template unavailable";return false;}
    // AnmManager::create(bank, script, -1, 0, at): no layer change, the position, a first tick.
    AnmVm* vm=adopt(*source);vm->visual.render_flags|=0x400;vm->visual.translation=at;
    if(vm->tick(random,1)<0){error=vm->error;stop();return false;}
    running=true;return true;
}
AnmVm* LoadingOverlay::spawn_child(AnmVm& source,i32 script,u32 ordering){
    // AnmManager::spawn_child: placed and turned with the parent, its first tick at once.
    const AnmVm* t=manager&&script>=0?manager->script_template(bank,u32(script)):nullptr;if(!t)return nullptr;
    AnmVm* vm=adopt(*t);vm->creation_parent=&source;vm->rotation_parent=source.rotation_parent?source.rotation_parent:&source;
    vm->visual.render_flags^=(source.visual.render_flags^vm->visual.render_flags)&0x2000000;
    if(vm->tick(random,1)<0){error=vm->error;return nullptr;}vm->ordering=ordering;return vm;
}
AnmVm* LoadingOverlay::spawn_detached(AnmVm& source,i32 script){
    // AnmManager::spawn_detached: where the source is now, on its own from then on.
    const AnmVm* t=manager&&script>=0?manager->script_template(bank,u32(script)):nullptr;if(!t)return nullptr;
    AnmVm* vm=adopt(*t);vm->visual.render_flags|=0x400;vm->visual.layer=source.visual.layer;vm->visual.render_flags^=(source.visual.render_flags^vm->visual.render_flags)&0x2000000;
    vm->visual.translation=source.visual.translation;vm->variables.rotation=source.variables.rotation;vm->visual.child_anchor=source.variables.position;
    if(vm->tick(random,1)<0){error=vm->error;return nullptr;}vm->ordering=0;return vm;
}
bool LoadingOverlay::advance(u32 frames){
    if(!running)return true;
    for(u32 f=0;f<frames;f++){
        // The VMs there at the start of the frame, in creation order (children made now had their first tick).
        Slot* order[capacity];u32 count=0;for(auto& s:slots)if(s.used)order[count++]=&s;
        std::sort(order,order+count,[](const Slot* a,const Slot* b){return a->order<b->order;});
        for(u32 i=0;i<count;i++){Slot& s=*order[i];if(!s.used)continue;
            const int r=s.vm.tick(random,1);
            if(r<0){error=s.vm.error;stop();return false;}
            if(r==1){const AnmVm* gone=&s.vm;for(auto& other:slots)if(other.used){if(other.vm.creation_parent==gone)other.vm.creation_parent=nullptr;if(other.vm.rotation_parent==gone)other.vm.rotation_parent=nullptr;}s.vm=AnmVm{};s.used=false;}
        }
    }
    return true;
}
u32 LoadingOverlay::quads(LoadingQuad* out,u32 room,GraphicsDevice& graphics){
    if(!running)return 0;u32 n=0;
    Slot* order[capacity];u32 count=0;for(auto& s:slots)if(s.used)order[count++]=&s;
    std::sort(order,order+count,[](const Slot* a,const Slot* b){return a->order<b->order;});
    for(u32 i=0;i<count;i++){AnmVm& vm=order[i]->vm;
        // AnmRenderer::draw's tests: shown, not hidden by render flags, not fully transparent.
        const auto& v=vm.visual;if(n==room)break;if((v.flags&3)!=3||(v.render_flags&0x60)||!vm.sprite||!(v.color>>24))continue;
        const AnmResource* source=vm.sprite_resource?vm.sprite_resource:vm.resource;if(!source)continue;
        LoadingQuad q;const u32 mode=v.draw_mode();
        if(mode<=3){   // AnmRenderer::quad: the sprite's four corners on the screen
            Vec3 p[4];if(!anm_quad_positions(vm,p))continue;
            // UVs as AnmRenderer::pack_quad builds them (scrolled and scaled).
            const Vec2 uv[4]={{float(v.uv_offset.x+v.uv[0].x),float(v.uv[0].y+v.uv_offset.y)},{float(float(float(v.uv[1].x-v.uv[0].x)*v.uv_scale.x)+v.uv_offset.x)+v.uv[0].x,float(v.uv_offset.y+v.uv[1].y)},
                {float(v.uv_offset.x+v.uv[2].x),float(float(float(v.uv[2].y-v.uv[0].y)*v.uv_scale.y)+v.uv_offset.y)+v.uv[0].y},{float(float(float(v.uv[3].x-v.uv[2].x)*v.uv_scale.x)+v.uv_offset.x)+v.uv[2].x,float(float(float(v.uv[3].y-v.uv[1].y)*v.uv_scale.y)+v.uv_offset.y)+v.uv[1].y}};
            for(u32 k=0;k<4;k++){q.x[k]=p[k].x;q.y[k]=p[k].y;q.u[k]=uv[k].x;q.v[k]=uv[k].y;}
        }else if(mode==8||mode==15){   // AnmRenderer::world_quad: the unit sprite through the world and texture matrices
            Matrix4 world;if(!anm_world_quad_transform(vm,world))continue;const u32 ax=v.flags>>21&3,ay=v.flags>>23&3;if(ax>2||ay>2)continue;
            constexpr float xs[3][2]={{-128,128},{0,256},{-256,0}},ys[3][2]={{-128,128},{0,256},{-256,0}};const Matrix4 t=anm_texture_transform(vm);
            for(u32 k=0;k<4;k++){const float x=xs[ax][k&1],y=ys[ay][k>>1],u=float(k&1),w=float(k>>1);
                q.x[k]=float(float(x*world.m[0])+float(y*world.m[4]))+world.m[12];q.y[k]=float(float(x*world.m[1])+float(y*world.m[5]))+world.m[13];
                q.u[k]=float(u*t.m[0])+t.m[8];q.v[k]=float(w*t.m[5])+t.m[9];}
        }else continue;
        q.color=v.color;q.blend=v.blend_mode();q.texture=graphics.texture(*source,vm.sprite->texture);q.layer=v.layer;out[n++]=q;
    }
    std::stable_sort(out,out+n,[](const LoadingQuad& a,const LoadingQuad& b){return a.layer<b.layer;});
    return n;
}
}
