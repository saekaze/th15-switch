// Host-side tests for the Switch port layer (Linux; no game data needed).
//   thbgm stream - PCM offsets, loop points, two tracks sharing one FILE
//   font baker   - T15G v3 tables accepted by the upstream GlyphAtlas and
//                  drawn by the upstream TextRaster (needs a CJK font)
//   checkpoint   - Pointdevice checkpoints encode + write on a background
//                  thread and read back intact (the read waits for it)
//   renderer     - the SDL2/GLES3 port of the shared renderer draws, reads
//                  back and presents (needs a display, e.g. Xvfb)
//   loading      - the start-up screen spreads sig.anm's picture over the
//                  4:3 screen from its textures' used areas and draws the
//                  sprites ascii.anm script 17 makes (stand-in files)
#include "../game/Types.hpp"
#include "../game/TextRaster.hpp"
#include "../src/BgmStream.hpp"
#include "../src/FontBaker.hpp"
#include "../src/FileStore.hpp"
#include "../game/CheckpointFile.hpp"
#include "../src/Platform.hpp"
#include "../portable/sdl/Renderer.hpp"
#include "../src/ApplicationState.hpp"
#include "../src/LoadingScreen.hpp"
#include <SDL.h>
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace th15;
static int failures=0;
#define CHECK(cond) do{if(!(cond)){std::printf("  FAIL %s:%d: %s\n",__FILE__,__LINE__,#cond);++failures;}}while(0)

static void test_bgm_stream(const std::string& dir){
    std::puts("thbgm stream");
    // Two tracks back to back: A = frames 0..9999 (value n), B = 4000 frames (value 20000+n).
    const std::string path=dir+"/thbgm.dat";const u32 offset_a=16,frames_a=10000,intro_a=2500,offset_b=offset_a+frames_a*4,frames_b=4000,intro_b=1000;
    {FILE* f=std::fopen(path.c_str(),"wb");std::fwrite("ZWAV",1,4,f);for(u32 i=4;i<offset_a;++i)std::fputc(0,f);
     for(u32 n=0;n<frames_a;++n){const i16 l=i16(n),r=i16(-i32(n));std::fwrite(&l,2,1,f);std::fwrite(&r,2,1,f);}
     for(u32 n=0;n<frames_b;++n){const i16 l=i16(20000+n),r=i16(-20000-i32(n));std::fwrite(&l,2,1,f);std::fwrite(&r,2,1,f);}std::fclose(f);}
    host::BgmFile file;file.file=std::fopen(path.c_str(),"rb");
    auto open=[&](host::PcmStream& s,u32 offset,u32 frames,u32 intro){
        s.source=&file;s.offset=offset;s.frames=frames;s.channels=2;s.rate=44100;
        auto config=ma_data_source_config_init();config.vtable=&host::pcm_vtable;
        CHECK(ma_data_source_init(&config,&s.base)==MA_SUCCESS);CHECK(ma_data_source_seek_to_pcm_frame(&s,0)==MA_SUCCESS);
        CHECK(ma_data_source_set_loop_point_in_pcm_frames(&s,intro,frames)==MA_SUCCESS);CHECK(ma_data_source_set_looping(&s,MA_TRUE)==MA_SUCCESS);
    };
    host::PcmStream a,b;open(a,offset_a,frames_a,intro_a);open(b,offset_b,frames_b,intro_b);
    // Interleave reads of both tracks, as two cached music voices would.
    std::vector<float> pa,pb;ma_uint64 read=0;
    for(u32 chunk=0;chunk<25;++chunk){std::vector<float> buf(2*1000);
        CHECK(ma_data_source_read_pcm_frames(&a,buf.data(),1000,&read)==MA_SUCCESS&&read==1000);pa.insert(pa.end(),buf.begin(),buf.end());
        CHECK(ma_data_source_read_pcm_frames(&b,buf.data(),1000,&read)==MA_SUCCESS&&read==1000);pb.insert(pb.end(),buf.begin(),buf.end());}
    auto ordered=[](const std::vector<float>& pcm,u32 base,u32 frames,u32 intro){
        for(u32 i=0;i<pcm.size()/2;++i){const u32 expect=base+(i<frames?i:intro+(i-frames)%(frames-intro));
            if(pcm[i*2]!=float(i16(expect))/32768.f||pcm[i*2+1]!=float(i16(-i32(expect)))/32768.f){std::printf("  frame %u: got %f expected %u\n",i,pcm[i*2]*32768.f,expect);return false;}}
        return true;};
    CHECK(ordered(pa,0,frames_a,intro_a));CHECK(ordered(pb,20000,frames_b,intro_b));
    CHECK(ma_data_source_seek_to_pcm_frame(&a,frames_a+1)!=MA_SUCCESS);
    ma_data_source_uninit(&a.base);ma_data_source_uninit(&b.base);std::fclose(file.file);
}

static bool test_font_baker(const std::string& dir){
    std::puts("font baker");
    const char* candidates[]={"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc","/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc"};
    std::string font;for(const char* c:candidates)if(host::exists(c)){font=c;break;}
    if(font.empty()){std::puts("  skipped: no CJK font installed");return false;}
    std::vector<u8> bytes;CHECK(host::read_file(font,bytes,64u*1024*1024));CHECK(host::write_file(dir+"/msgothic.ttc",bytes));
    host::paths().data=host::paths().save=dir;
    host::BakedFonts baked;std::string error;const double begin=host::seconds();
    const bool ok=host::bake_fonts(baked,error);CHECK(ok);if(!ok){std::printf("  %s\n",error.c_str());return true;}
    size_t total=0;for(const auto& f:baked.files)total+=f.size();
    std::printf("  baked 8 fonts in %.2fs (%zu bytes)\n",host::seconds()-begin,total);
    TextRaster raster;
    for(u32 n=0;n<15;++n){if(baked.files[n].empty())continue;const bool loaded=raster.glyphs.load(n,baked.files[n].data(),u32(baked.files[n].size()));if(!loaded)std::printf("  table %u rejected\n",n);CHECK(loaded);}
    CHECK(raster.glyphs.ready());
    // "東方紺珠伝" in Shift-JIS, drawn through the upstream text path for every
    // style (0-8) in both font modes.
    const std::string text="\x93\x8c\x95\xfb\x8d\xa8\x8e\xec\x93\x60";
    for(i32 style_index=0;style_index<=8;++style_index)for(bool fallback:{false}){
        TextStyle style;style.height=24;style.font=style_index;style.color=0xffffff;style.outline=0x000000;style.fallback=fallback;
        const bool drawn=raster.rasterize(text,style);CHECK(drawn);
        u32 lit=0;const auto* px=reinterpret_cast<const u16*>(raster.scratch.pixels.data());
        for(u32 n=0;n<raster.scratch.pixels.size()/2;++n)if((px[n]&0xf000)&&(px[n]&0x0fff))++lit;
        if(!fallback)std::printf("  style %d (font %u): %u lit texels\n",style_index,TextRaster::font_index(style_index,false),lit);
        CHECK(lit>200);
    }
    return true;
}

namespace {
struct Surfaces {std::map<u32,std::vector<u8>> pixels;std::map<u32,touhou::sdl::Surface> info;};
touhou::sdl::Surface resolve(void* owner,u32 id){auto& s=*static_cast<Surfaces*>(owner);auto it=s.info.find(id);if(it==s.info.end())return {};auto r=it->second;auto& p=s.pixels[id];r.data=p.data();r.size=u32(p.size());return r;}
void add(Surfaces& s,u32 id,u32 w,u32 h,touhou::graphics::PixelFormat f,u32 bpp){s.pixels[id].assign(size_t(w)*h*bpp,0);touhou::sdl::Surface surface;surface.handle=id;surface.width=w;surface.height=h;surface.format=f;surface.pitch=w*bpp;s.info[id]=surface;}
}
static bool test_renderer(){
    std::puts("renderer (SDL2 + GLES3)");
    if(!std::getenv("DISPLAY")&&!std::getenv("WAYLAND_DISPLAY")){std::puts("  skipped: no display");return false;}
    using namespace touhou::graphics;
    Surfaces surfaces;add(surfaces,1,640,480,PixelFormat::Bgra8,4);add(surfaces,2,640,480,PixelFormat::Depth16,2);add(surfaces,3,2,2,PixelFormat::Bgra8,4);
    for(u32 n=0;n<4;++n){auto* p=surfaces.pixels[3].data()+n*4;p[0]=0;p[1]=255;p[2]=0;p[3]=255;} // green texture
    touhou::sdl::Renderer r(10,resolve,&surfaces);r.title="th15-switch test";
    const bool ok=r.initialize();CHECK(ok);if(!ok){std::printf("  %s\n",r.error());return true;}
    r.state.target=1;r.state.depth=2;r.viewport({0,0,640,480,0,1});
    r.clear(3,0xff0000ff,1,0); // blue
    struct V {float x,y,z,w;u32 diffuse;float u,v;};
    const V quad[4]{{100,100,0,1,0xffffffff,0,0},{200,100,0,1,0xffffffff,1,0},{100,200,0,1,0xffffffff,0,1},{200,200,0,1,0xffffffff,1,1}};
    auto& p=r.pipeline();p=PipelineState{};p.depthTest=false;p.color.operation=p.alpha.operation=ColorOperation::First;p.color.first=p.alpha.first={ArgumentSource::Texture};
    r.state.texture=3;r.state.layout=attributes(VertexLayout::ScreenColorUv);
    r.draw(Topology::Strip,2,quad,sizeof(V));
    r.read(1);
    auto pixel=[&](u32 x,u32 y){const auto* q=surfaces.pixels[1].data()+(y*640+x)*4;return u32(q[2])<<16|u32(q[1])<<8|q[0];};
    std::printf("  centre %06x corner %06x\n",pixel(150,150),pixel(10,10));
    CHECK(pixel(150,150)==0x00ff00);CHECK(pixel(10,10)==0x0000ff);CHECK(pixel(99,150)==0x0000ff);CHECK(pixel(150,99)==0x0000ff);
    ++surfaces.info[1].version;r.present(1);
    std::printf("  picture %dx%d at %d,%d in %dx%d\n",r.picture.width,r.picture.height,r.picture.x,r.picture.y,r.picture.drawable_width,r.picture.drawable_height);
    CHECK(r.picture.width*3==r.picture.height*4);CHECK(r.picture.x*2+r.picture.width==r.picture.drawable_width);
    CHECK(glGetError()==GL_NO_ERROR);
    return true;
}

// One ANM v8 chunk: a w x h texture (pw x ph of it in `color`, the rest green),
// optional sprites (x, y, width, height) and scripts.
static void anm_chunk(std::vector<u8>& anm,u16 w,u16 h,u16 pw,u16 ph,u32 color,const std::vector<std::array<float,4>>& sprites,const std::vector<std::vector<u8>>& scripts,bool last){
    const u32 ns=u32(sprites.size()),nc=u32(scripts.size()),table_end=64+ns*4+nc*8,name=table_end,first_sprite=name+8;u32 at=first_sprite+ns*20;std::vector<u32> script_at;
    for(const auto& code:scripts){script_at.push_back(at);at+=u32(code.size());}
    const u32 thtx=at,bytes=u32(w)*h*4,size=thtx+16+bytes,base=u32(anm.size());anm.resize(base+size,0);u8* c=anm.data()+base;
    const auto put=[&](u32 at,u32 v,u32 n){for(u32 i=0;i<n;++i)c[at+i]=u8(v>>(8*i));};
    put(0,8,4);put(4,ns,2);put(6,nc,2);put(10,w,2);put(12,h,2);put(14,1,2);put(16,name,4);put(28,thtx,4);c[32]=1;put(36,last?0:size,4);std::memcpy(c+name,"tex",3);
    for(u32 i=0;i<ns;++i){const u32 o=first_sprite+i*20;put(64+i*4,o,4);put(o,i,4);std::memcpy(c+o+4,sprites[i].data(),16);}
    for(u32 i=0;i<nc;++i){put(64+ns*4+i*8,i,4);put(64+ns*4+i*8+4,script_at[i],4);std::memcpy(c+script_at[i],scripts[i].data(),scripts[i].size());}
    std::memcpy(c+thtx,"THTX",4);put(thtx+6,1,2);put(thtx+8,w,2);put(thtx+10,h,2);put(thtx+12,bytes,4);
    for(u32 y=0;y<h;++y)for(u32 x=0;x<w;++x)put(thtx+16+(y*w+x)*4,y<ph&&x<pw?color:0xff00ff00,4);
}
// One ANM instruction: opcode, time, integer arguments.
static void anm_op(std::vector<u8>& code,i16 opcode,i16 time,std::initializer_list<i32> args={}){
    const u32 at=u32(code.size()),length=8+u32(args.size())*4;code.resize(at+length,0);
    const auto put=[&](u32 o,u32 v,u32 n){for(u32 i=0;i<n;++i)code[at+o+i]=u8(v>>(8*i));};put(0,u16(opcode),2);put(2,length,2);put(4,u16(time),2);u32 o=8;for(i32 a:args){put(o,u32(a),4);o+=4;}
}

static void test_loading_screen(){
    std::puts("loading screen (sig.anm picture, ascii.anm script 17)");
    auto app=std::make_unique<sdl::ApplicationState>();
    if(!app->graphics.initialize()){std::printf("  skipped: %s\n",app->graphics.error.c_str());return;}
    auto& gpu=app->graphics.backend;
    const auto frame=[&](sdl::LoadingScreen& screen,u32 frames){for(u32 i=0;i<frames;++i){SDL_Delay(17);screen.pump();}gpu.flush();gpu.read(sdl::GraphicsDevice::screen);};
    const auto* image=app->graphics.pixels(sdl::GraphicsDevice::screen);
    auto pixel=[&](u32 x,u32 y){const auto* q=image->pixels.data()+y*image->pitch+x*4;return u32(q[2])<<16|u32(q[1])<<8|q[0];};
    // Stand-in sig.anm laid out like v1.00b's (1280x960 over a 1024x1024 and a
    // 256x1024 texture, 960 rows used): 80x60 over 64x64 (red) and 16x64
    // (blue). The unused rows are green and must not show.
    std::vector<u8> sig;anm_chunk(sig,64,64,64,60,0xffff0000,{},{},false);anm_chunk(sig,16,64,16,60,0xff0000ff,{},{},true);
    CHECK(app->animations.load(1,sig.data(),u32(sig.size())));CHECK(app->graphics.preload(*app->animations.resource(1)));
    {sdl::LoadingScreen screen{*app};screen.start();frame(screen,2);screen.stop();}   // no ascii.anm yet: the picture alone
    std::printf("  picture: left %06x right %06x bottom %06x/%06x\n",pixel(250,240),pixel(580,240),pixel(250,470),pixel(580,470));
    CHECK(pixel(4,4)==0xff0000);CHECK(pixel(250,240)==0xff0000);CHECK(pixel(505,240)==0xff0000);CHECK(pixel(520,240)==0x0000ff);CHECK(pixel(635,240)==0x0000ff);
    CHECK(pixel(250,470)==0xff0000);CHECK(pixel(580,470)==0x0000ff);CHECK(pixel(480,392)==0xff0000);
    // Stand-in ascii.anm: script 17 makes script 16 (a white 32x32 sprite) as
    // its child on frame 1, as the real one makes its texts and moons. Both
    // use 1280x960 coordinates (op 313 mode 2: halved), as TH15's scripts do.
    std::vector<std::vector<u8>> scripts(18);for(u32 i=0;i<16;++i)anm_op(scripts[i],-1,0);
    anm_op(scripts[16],313,0,{2});anm_op(scripts[16],300,0,{0});anm_op(scripts[16],2,0);anm_op(scripts[16],-1,0);
    anm_op(scripts[17],313,0,{2});anm_op(scripts[17],501,1,{16});anm_op(scripts[17],2,1);anm_op(scripts[17],-1,1);
    std::vector<u8> ascii;anm_chunk(ascii,32,32,32,32,0xffffffff,{{0,0,32,32}},scripts,true);
    CHECK(app->animations.load(2,ascii.data(),u32(ascii.size())));CHECK(app->graphics.preload(*app->animations.resource(2)));
    sdl::LoadingScreen screen{*app};screen.start();frame(screen,0);CHECK(pixel(480,392)==0xff0000);   // frame 0: nothing made yet
    frame(screen,3);u32 left=640,right=0,top=480,bottom=0;
    for(u32 y=0;y<480;++y)for(u32 x=0;x<640;++x)if(pixel(x,y)==0xffffff){left=std::min(left,x);right=std::max(right,x);top=std::min(top,y);bottom=std::max(bottom,y);}
    std::printf("  child sprite at x %u-%u, y %u-%u\n",left,right,top,bottom);
    CHECK(left>=468&&left<=474&&right>=485&&right<=491&&top>=380&&top<=386&&bottom>=397&&bottom<=403);   // 16x16 around (960,784)/2
    screen.stop();CHECK(glGetError()==GL_NO_ERROR);
}

static void test_checkpoint(const std::string& dir){
    std::puts("checkpoint (background save)");
    host::make_directory(dir+"/save");host::make_directory(dir+"/save/autosave");
    sdl::FileStore files(dir+"/save");
    auto file=std::make_shared<CheckpointFile>();
    file->header=CheckpointHeader::create(1700000000,2,3,4,5,{},0);
    for(u32 i=0;i<file->sections.size();i++){const u32 n=i==0?0x1cc:i==7?0x514:i*40000u;file->sections[i].resize(n);for(u32 k=0;k<n;k++)file->sections[i][k]=u8((k*7+i)%13==0?k:i);}
    const auto expected=file->sections;
    std::vector<u8> bytes;const double begin=host::seconds();
    files.save_checkpoint_async(2,3,bytes,[file](std::vector<u8>& out,std::string& failure){if(file->encode(out))return true;failure=file->error;return false;});
    const double queued=host::seconds()-begin;
    std::vector<u8> read;bool present=false;CHECK(files.checkpoint(2,3,read,present));CHECK(present);
    std::printf("  queued in %.2f ms, written and read back in %.0f ms (%zu bytes)\n",queued*1000,(host::seconds()-begin)*1000,read.size());
    CHECK(queued<0.01);CHECK(read==bytes);
    CheckpointFile back;CHECK(back.open(read.data(),u32(read.size())));CHECK(back.sections==expected);CHECK(back.header.character()==2&&back.header.difficulty()==3);
}

int main(){
    const std::string dir=std::string(std::getenv("TMPDIR")?std::getenv("TMPDIR"):"/tmp")+"/th15-switch-test";
    host::make_directory(dir);
    test_bgm_stream(dir);test_font_baker(dir);test_checkpoint(dir);
    if(SDL_Init(SDL_INIT_VIDEO)==0){test_renderer();SDL_Quit();}else std::printf("renderer skipped: %s\n",SDL_GetError());
    if(SDL_Init(SDL_INIT_VIDEO)==0){test_loading_screen();SDL_Quit();}
    std::printf(failures?"FAILED (%d)\n":"all host tests passed\n",failures);
    return failures?1:0;
}
