#include "FontDevice.hpp"
#include "FontBaker.hpp"
#include "Platform.hpp"
#include <cstdio>
namespace th15::sdl {
namespace {
std::string file_name(u32 slot){return slot<13?"font"+std::to_string(slot)+".bin":slot==13?"cp932.bin":"blend4444.bin";}
// Bumped whenever the baker output changes; also keyed on the source font.
std::string bake_stamp(){
    std::string stamp="th15-switch-fontbake-2";const auto path=host::paths().data_file("msgothic.ttc");
    FILE* f=std::fopen(path.c_str(),"rb");long size=-1;if(f){std::fseek(f,0,SEEK_END);size=std::ftell(f);std::fclose(f);}
    return stamp+" msgothic.ttc="+std::to_string(size);
}
}
// Switch port: upstream loads each table lazily from /fonts/ (baked offline
// with Windows GDI). Here all fifteen come, in order, from:
//   1. <data>/fonts/ - those GDI tables, if copied from an upstream web build;
//   2. <save>/fontcache/ - a previous FreeType bake (see FontBaker.hpp);
//   3. a fresh FreeType bake, written to fontcache/ for the next launch.
bool FontDevice::load(u32 slot){if(slot>=loaded.size()){error="Font slot outside range";return false;}if(!loaded[slot]){error="Font table "+file_name(slot)+" unavailable";return false;}return true;}
bool FontDevice::initialize(){
    if(loaded[13]&&loaded[14])return true;
    auto load_dir=[&](const std::string& dir){
        // Fonts 0-7 plus the two shared tables are required; 8-12 (fallback
        // mode only) are used when present.
        auto needed=[](u32 n){return n<8||n>=13;};std::vector<u8> data[15];
        for(u32 n=0;n<15;++n)if(!host::read_file(dir+"/"+file_name(n),data[n],32*1024*1024)&&needed(n))return false;
        TextRaster fresh;for(u32 n=0;n<15;++n)if(!data[n].empty()&&!fresh.glyphs.load(n,data[n].data(),u32(data[n].size()))&&needed(n))return false;
        for(u32 n=0;n<15;++n)loaded[n]=!data[n].empty()&&raster.glyphs.load(n,data[n].data(),u32(data[n].size()));return raster.glyphs.ready();
    };
    const auto original=host::paths().data_file("fonts"),cache=host::paths().save_file("fontcache");
    if(load_dir(original)){host::log("fonts: original GDI tables from %s",original.c_str());return true;}
    const auto stamp=bake_stamp();std::vector<u8> saved;
    if(host::read_file(cache+"/stamp.txt",saved,4096)&&std::string(saved.begin(),saved.end())==stamp&&load_dir(cache)){host::log("fonts: cached bake");return true;}
    host::log("fonts: baking glyph tables (first launch only)...");const double begin=host::seconds();
    host::BakedFonts baked;if(!host::bake_fonts(baked,error,progress))return false;
    for(u32 n=0;n<15;++n)if(!baked.files[n].empty()&&!(loaded[n]=raster.glyphs.load(n,baked.files[n].data(),u32(baked.files[n].size())))){error="Baked font table rejected: "+file_name(n);return false;}
    host::log("fonts: baked in %.1fs from %s",host::seconds()-begin,baked.source.c_str());
    if(host::make_directory(cache)){bool ok=true;for(u32 n=0;n<15;++n)if(!baked.files[n].empty())ok=ok&&host::write_file(cache+"/"+file_name(n),baked.files[n]);
        if(ok)host::write_file(cache+"/stamp.txt",reinterpret_cast<const u8*>(stamp.data()),stamp.size());}
    return raster.glyphs.ready();
}
bool FontDevice::preload(i32 style){return initialize()&&load(TextRaster::font_index(style,fallback));}
bool FontDevice::text(AnmVm& vm,const DialogueText& request){
    TextLayout layout;if(!TextLayout::dialogue(vm,request,fallback,layout)){error="Missing text animation or style";return false;}if(!preload(request.font))return false;const u32 handle=graphics.texture(*layout.resource,layout.texture);auto* image=graphics.pixels(handle);if(!image){error="Text texture was not prepared";return false;}
    if(!raster.write(*image,layout.region,request.bytes,layout.style)){error="Original text raster bounds or format";return false;}graphics.changed(handle,{u32(layout.region.left),u32(layout.region.top),u32(layout.region.right),u32(layout.region.bottom)});vm.visual.flags|=1;++writes;return true;
}
}
