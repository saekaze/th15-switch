#include "Dialogue.hpp"
#include <algorithm>
#include <cstdlib>
namespace th15 {
bool Dialogue::check(bool ok,const char* message){if(!ok&&error.empty())error=message;return ok;}
bool Dialogue::remove_balloon(){return check(host.retire(state.handles[11]),"Dialogue balloon retirement failed");}
bool Dialogue::reset_lines(){state.line=0;state.box_visible=0;return true;}
bool Dialogue::reset_depth(){for(u32 i=6;i<10;i++)if(!check(host.depth(state.handles[i],0),"Dialogue text depth failed"))return false;state.flags&=~2u;return reset_lines();}
bool Dialogue::select_speaker(i32 speaker){state.speaker=speaker;return reset_depth();}
bool Dialogue::initialize(){
    state={};error.clear();ended=false;state.age.set(0);state.clock.set(0);state.wait.set(0);
    for(u32 i=0;i<4;i++){auto& h=state.handles[6+i];if(!check(host.create(resources.text,i<2?0:1,h),"Dialogue text animation creation failed"))return false;if(i&1)if(!check(host.interrupt(h,7),"Dialogue text initial interrupt failed"))return false;if(!check(host.text_initialize(h),"Dialogue text initialization failed"))return false;}
    return check(host.clear_field(),"Dialogue field clear failed");
}
bool Dialogue::draw_line(const MessageInstruction& c){
    const std::string text=c.text();const u32 handle=state.handles[6+state.line];const u32 ruby=state.handles[8+state.line];
    const auto font=state.flags&2?1:0;const u32 color=state.colors[std::clamp(state.speaker,0,2)];
    if(!state.line&&!state.box_visible){state.width=0;for(u32 i=0;i<4;i++){DialogueText clear{state.handles[6+i],"  ",font,i<2?0:1,color};if(!check(host.text(clear),"Dialogue text clear failed"))return false;}state.box_visible=1;for(u32 i=6;i<10;i++)if(!check(host.interrupt(state.handles[i],3),"Dialogue box interrupt failed"))return false;}
    if(!text.empty()&&text[0]=='|'){char* end=nullptr;const auto a=std::strtol(text.c_str()+1,&end,10);if(!end||*end!=',')return check(false,"Invalid MSG ruby width");const auto b=std::strtol(end+1,&end,10);if(!end||*end!=',')return check(false,"Invalid MSG ruby offset");DialogueText request{ruby,std::string(end+1),2,i32(b),0,state.position,true,{i32(a),i32(b)},0xa0a0a0,i32(a)};return check(host.text(request)&&host.interrupt(ruby,2,state.line==0),"Dialogue ruby text failed");}
    const u32 raw=(u32(text.size())*8u&~15u)-28u;state.width=std::max(float(double(raw)*2.),state.width);const i32 style=state.speaker+i32((state.flags>>2&15)*2)+(state.line?8:0);
    if(!remove_balloon())return false;if(!check(host.create_balloon(resources.balloons,style+221,state.position,state.width,style,state.handles[11]),"Dialogue balloon creation failed"))return false;state.balloon_style=style;
    DialogueText request{handle,text,font,0,color,state.position,true};if(!check(host.text(request),"Dialogue text painting failed"))return false;
    if(state.speaker>=1){for(u32 i=6;i<10;i++)if(!check(host.position(state.handles[i],state.position),"Dialogue text position failed"))return false;}
    if(!check(host.interrupt(handle,2,true),"Dialogue text appearance failed"))return false;state.line=state.line?0:1;if(!state.line)state.box_visible=0;return true;
}
bool Dialogue::execute(const MessageInstruction& c){
    const i32 a=c.argument<i32>(0),b=c.argument<i32>(1);
    auto interrupt=[&](u32 slot,i32 label,bool immediate=false){return check(host.interrupt(state.handles[slot],label,immediate),"Dialogue animation interrupt failed");};
    auto portrait=[&](u32 slot,DialoguePortrait p){return check(host.create(p.resource,p.script,state.handles[slot]),"Dialogue portrait creation failed");};
    auto valid=[&](i32 slot){return check(slot>=0&&slot<4,"Invalid dialogue portrait slot");};
    switch(c.opcode){
    case 1:{constexpr i32 scripts[]={27,34,23,20};if(!check(resources.character>=0&&resources.character<4,"Invalid dialogue character"))return false;return portrait(0,{resources.player,scripts[resources.character]});}
    case 2:if(!valid(a)||!portrait(1+u32(a),resources.enemies[u32(a)]))return false;state.portrait_state=0;return true;
    case 4:if(!interrupt(0,1))return false;state.handles[0]=0;return true;
    case 5:if(!valid(a)||!interrupt(1+u32(a),1))return false;state.handles[1+u32(a)]=0;return interrupt(10,1);
    case 6:for(u32 i=6;i<10;i++)if(!interrupt(i,1))return false;return remove_balloon();
    case 7:for(u32 i=1;i<5;i++)if(!interrupt(i,3,true))return false;return interrupt(0,2,true)&&interrupt(5,2)&&select_speaker(0);
    case 8:if(!valid(a)||!interrupt(0,3,true)||!interrupt(1+u32(a),2,true)||!interrupt(5,3))return false;return select_speaker(1);
    case 9:if(!interrupt(0,3,true))return false;for(u32 i=1;i<5;i++)if(!interrupt(i,3,true))return false;if(!interrupt(5,3))return false;state.speaker=0;for(u32 i=6;i<10;i++)if(!check(host.position(state.handles[i],state.anchors[0]),"Dialogue anchor restore failed"))return false;return reset_depth();
    case 10:state.flags=(state.flags&~1u)|(u32(c.payload.empty()?0:c.payload[0])&1);return true;
    case 12:state.complete=1;return true;
    case 13:return interrupt(0,wrapping_add(a,17),true);
    case 14:if(!valid(b))return false;return interrupt(1+u32(b),wrapping_add(a,17),true);
    case 15:case 16:{const u32 slot=6+c.opcode-15;DialogueText request{state.handles[slot],c.text(),4+i32(state.flags&2?1:0),0,state.colors[std::clamp(state.speaker,0,2)]};return check(host.text(request),"Dialogue standalone text failed")&&interrupt(slot,2);}
    case 17:return draw_line(c);
    case 18:if(!remove_balloon())return false;for(u32 i=6;i<10;i++)if(!interrupt(i,3))return false;return true;
    case 19:{u32 logo=0;return check(host.music(true),"Dialogue boss BGM failed")&&check(host.create(resources.logo,2,logo),"Dialogue music logo failed");}
    case 20:if(!valid(a)||!portrait(10,resources.names[u32(a)]))return false;return check(host.name_banner(),"Dialogue enemy name banner failed");
    case 21:return check(host.stage_complete(),"Dialogue stage completion failed");
    case 22:return check(host.fade_music(input_stage_==6?2.f:8.f),"Dialogue music fade failed");
    case 23:return interrupt(0,7);
    case 24:return interrupt(1,7)&&interrupt(2,7);
    case 25:for(u32 i=6;i<10;i++)if(!check(host.depth(state.handles[i],float(a)),"Dialogue depth failed"))return false;return true;
    case 26:state.flags|=2;return true;
    case 27:return check(host.fade_music(c.argument<float>(0)),"Dialogue explicit music fade failed");
    case 28:state.position.x=float(c.argument<float>(0)*2.f);state.position.y=float(c.argument<float>(1)*2.f);return true;
    case 29:state.flags=(state.flags&~0x3cu)|((u32(a)<<2)&0x3c);return true;
    case 31:if(!portrait(2,resources.enemies[1]))return false;state.portrait_state=0;return true;
    case 32:if(!check(a>=0&&a<3,"Invalid dialogue speaker")||!interrupt(5,3))return false;return select_speaker(a);
    default:return true; // Original 3, 30 and values above 32 simply advance.
    }
}
int Dialogue::update(const DialogueInput& input){
    if(!error.empty())return -2;if(ended)return -1;input_stage_=input.stage;if(state.complete>0)state.complete--;if(input.pressed&0x201)state.flags|=0x40;
    if(state.cursor>=script.instructions.size())return check(false,"Dialogue cursor escaped MSG script"),-2;
    if((state.flags&0x41)==0x41&&(((input.held&0x200)&&input.skip_frames>=20)||((input.held&1)&&input.shoot_frames>=20)))state.clock.set(script.instructions[state.cursor].time);
    u32 budget=65536;while(state.cursor<script.instructions.size()&&state.clock.current>=script.instructions[state.cursor].time){
        if(!budget--)return check(false,"Dialogue instruction budget exhausted"),-2;const auto& c=script.instructions[state.cursor];
        if(c.opcode==0){ended=true;return -1;}
        if(c.opcode==11){if(state.wait.current<=0)state.wait.set(c.argument<i32>(0));state.wait.decrement(&input.rate);
            const bool pressed=input.pressed&0x80001;const bool skip=(state.flags&0x41)==0x41&&(input.skip_frames>=20||input.shoot_frames>=20);
            if(!pressed&&state.wait.current>0&&!skip)return check(host.follow_balloon(state),"Dialogue balloon tracking failed")?0:-2;
            if((pressed||state.wait.current<=0)&&!check(host.sound(0),"Dialogue advance sound failed"))return -2;state.wait.set(0);reset_lines();
        }else if(!execute(c))return -2;state.cursor++;
    }
    state.clock.tick(&input.rate);return check(host.follow_balloon(state),"Dialogue balloon tracking failed")?0:-2;
}
}
