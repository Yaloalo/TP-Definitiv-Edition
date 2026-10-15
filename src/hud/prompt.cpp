#include "hud/prompt.hpp"
#include "hud/draw_context.hpp"
#include "hud/idle_fade.hpp"
#include "core/hooks.hpp"
#include "m_Do/m_Do_ext.h"
#include "JSystem/JUtility/JUTFont.h"
#include <sstream>
#include <vector>

namespace remaster::hud {
namespace {
std::vector<std::string> g_prompts;
float text_width(JUTFont* font,std::string_view text,float size) {
    // JUTFont's draw boolean controls first-glyph spacing, NOT visibility.
    // Match drawChar_scale(..., true) advances without issuing any GX draw.
    float width=0;
    for (size_t i=0;i<text.size();++i) {
        unsigned code=static_cast<unsigned char>(text[i]);
        if (font->isLeadByte(code) && i+1<text.size()) code=(code<<8)|static_cast<unsigned char>(text[++i]);
        JUTFont::TWidth glyph{}; font->getWidthEntry(code,&glyph);
        width+=font->isFixed()?font->getFixedWidth():glyph.field_0x1;
    }
    return width*size/std::max(1,font->getCellWidth());
}
HookAction begin_prompts(ModContext*,void*,void*,void*) { g_prompts.clear(); return HOOK_CONTINUE; }
void draw_queued(ModContext*,void*,void*,void*) {
    auto* font=mDoExt_getSubFont(); if (!font) font=mDoExt_getMesgFont();
    if (!font || g_prompts.empty()) return;
    DrawContext context;
    const float width=std::clamp(mDoGph_gInf_c::getSafeWidthF()-340,200.0f,360.0f);
    const float center=(mDoGph_gInf_c::getSafeMinXF()+mDoGph_gInf_c::getSafeMaxXF())/2;
    std::vector<std::string> lines;
    for (const auto& prompt:g_prompts) {
        std::istringstream words(prompt);
        std::string word,line;
        while (words>>word) {
            const auto next=line.empty()?word:line+" "+word;
            if (!line.empty() && text_width(font,next,11)>width) { lines.push_back(line); line=word; }
            else line=next;
        }
        if (!line.empty()) lines.push_back(line);
    }
    g_prompts.clear();
    if (lines.empty()) return;
    float y=mDoGph_gInf_c::getSafeMaxYF()-38-float(lines.size()-1)*16;
    const u8 alpha=u8(idle_fade::opacity()*255);
    J2DFillBox(center-width/2-8,y-14,width+16,float(lines.size())*16+8,JUtility::TColor(20,22,26,u8(alpha*0.8f)));
    font->setGX(); font->setCharColor(JUtility::TColor(236,220,173,alpha));
    for (const auto& row:lines) { font->drawString_scale(center-text_width(font,row,11)/2,y,11,14,row.c_str(),true); y+=16; }
}
}
void draw_prompt(std::string_view text) {
    if (!text.empty() && std::find(g_prompts.begin(),g_prompts.end(),text)==g_prompts.end()) g_prompts.emplace_back(text);
}
ModResult install_prompts() {
    HookOptions first=HOOK_OPTIONS_INIT,last=HOOK_OPTIONS_INIT;
    first.priority=30000; last.priority=-30000;
    auto r=mods::hook::add_pre<hooks::Shared1>(begin_prompts,&first);
    if(r==MOD_OK)r=mods::hook::add_post<hooks::Shared1>(draw_queued,&last);
    return r;
}
}
