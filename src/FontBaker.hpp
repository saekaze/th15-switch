// Switch host: builds the glyph tables TH15's GlyphAtlas reads (T15G v3 fonts,
// the CP932 map and the 4-bit blend table) with FreeType.
//
// Upstream bakes these offline with Windows GDI (th15_web/scripts/
// prepare-fonts.mjs) because the original renders text with CreateFontA +
// TextOut. Here the same thirteen CreateFontA requests (heights 15-60,
// weights 400-700; upstream's measurement resolved every face to MS Gothic)
// are rasterised with FreeType from the user's msgothic.ttc. Coverage is
// quantised to 7 classes; blend4444 mixes those like GDI's antialiased text
// on ARGB4444.
#pragma once
#include "../game/Types.hpp"
#include <functional>
#include <string>
#include <vector>

namespace th15::host {
struct BakedFonts {
    std::vector<u8> files[15]; // font0..12.bin (8-12 left empty), cp932.bin, blend4444.bin
    std::string source;       // what was used, for the log
};
// Returns false with `error` set when no usable font was found.
// `progress` (optional) receives 0..1 while the glyphs are rasterised.
bool bake_fonts(BakedFonts& out,std::string& error,const std::function<void(float)>& progress=nullptr);
// The CP932 -> UTF-16 map the upstream baker derives from shift_jis.
const u16* cp932_table();
}
