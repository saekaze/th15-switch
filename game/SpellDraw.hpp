#pragma once
#include "SpellCard.hpp"
#include "AnmManager.hpp"
#include "RecordStore.hpp"
#include "HudDrawData.hpp"
namespace th15 {
struct SpellDrawContext {i32 character=0,subcharacter=0;u32 mode_flags=0;HudTextStyle style;};
bool draw_spell_card(SpellCard&,AnmManager&,const RecordStore&,SpellDrawContext&,HudDrawServices&,std::string& error);
}
