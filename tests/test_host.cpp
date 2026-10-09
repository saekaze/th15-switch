// Host-side tests for the Switch port layer (Linux; no game data needed).
//   thbgm stream - PCM offsets, loop points, two tracks sharing one FILE
//   font baker   - T15G v3 tables accepted by the upstream GlyphAtlas and
//                  drawn by the upstream TextRaster (needs a CJK font)
//   checkpoint   - Pointdevice checkpoints encode + write on a background
//                  thread and read back intact (the read waits for it)
//   renderer     - the SDL2/GLES3 port of the shared renderer draws, reads
//                  back and presents (needs a display, e.g. Xvfb)
#include "../game/Types.hpp"
#include "../game/TextRaster.hpp"
#include "../src/BgmStream.hpp"
#include "../src/FontBaker.hpp"
#include "../src/FileStore.hpp"
#include "../game/CheckpointFile.hpp"
#include "../src/Platform.hpp"
#include "../portable/sdl/Renderer.hpp"
#include <SDL.h>
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
    std::printf(failures?"FAILED (%d)\n":"all host tests passed\n",failures);
    return failures?1:0;
}
