#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace remaster::economy::dialogue {
struct Tag {size_t at,size;uint8_t group;uint16_t type;};
inline bool scan(std::span<const uint8_t> text,std::vector<Tag>& tags,size_t& length) {
    tags.clear();length=0;
    for(size_t at=0;at<text.size();) {
        if(text[at]==0){length=at+1;return true;}
        if(text[at]!=0x1a){++at;continue;}
        if(text.size()-at<5)return false;
        const auto size=text[at+1];if(size<5||size>text.size()-at)return false;
        tags.push_back({at,size,text[at+2],uint16_t(text[at+3]<<8|text[at+4])});at+=size;
    }
    return false;
}
inline std::string visible(std::span<const uint8_t> text) {
    std::vector<Tag> tags;size_t size=0;if(!scan(text,tags,size))return {};
    std::string out;size_t at=0;
    for(const auto& tag:tags){out.append(reinterpret_cast<const char*>(text.data()+at),tag.at-at);at=tag.at+tag.size;}
    out.append(reinterpret_cast<const char*>(text.data()+at),size-1-at);return out;
}
inline std::string lower(std::string text) {
    for(auto& c:text)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return text;
}
inline bool money(std::string_view text) {
    for(auto word:{"rupee","rubin","rubis","rupia","rupie"})if(text.find(word)!=text.npos)return true;
    return false;
}
inline bool input_prompt(std::span<const uint8_t> text) {
    std::vector<Tag> tags;size_t size=0;if(!scan(text,tags,size))return false;
    for(const auto& tag:tags)if(tag.group==0&&tag.type==37)return true;
    return false;
}
inline bool payment_tag(std::span<const uint8_t> text) {
    std::vector<Tag> tags;size_t size=0;if(!scan(text,tags,size))return false;
    for(const auto& tag:tags)if((tag.group==0&&(tag.type==37||tag.type==50))||(tag.group==5&&tag.type==9))return true;
    return false;
}
inline bool reword(std::span<const uint8_t> original,std::string_view body,
                   std::span<const char* const> options,std::vector<uint8_t>& out) {
    std::vector<Tag> tags;size_t length=0;if(!scan(original,tags,length))return false;
    out.clear();bool written=false,inChoice=false,labelWritten=false;size_t choice=0,at=0;
    const auto append=[&](std::string_view value){out.insert(out.end(),value.begin(),value.end());};
    const auto plain=[&](size_t stop) {
        if(stop==at)return;
        if(inChoice) {
            if(!labelWritten){append(choice<options.size()?options[choice]:"Continue");labelWritten=true;}
            // Preserve the option separator, never the obsolete price/amount label.
            for(size_t i=at;i<stop;++i)if(original[i]=='\n')out.push_back('\n');
        } else if(!written){append(body);written=true;}
    };
    for(const auto& tag:tags) {
        plain(tag.at);
        if(tag.group==0&&(tag.type==8||tag.type==9||tag.type==30||tag.type==31||tag.type==51||tag.type==52)) {
            if(inChoice)++choice;else{inChoice=true;choice=0;}
            labelWritten=false;
        }
        // These substitutions only render an old price/name; they do not run quests.
        const bool cosmetic=(tag.group==0&&(tag.type==0||tag.type==49||tag.type==50))||(tag.group==5&&tag.type==9);
        if(!cosmetic)out.insert(out.end(),original.begin()+tag.at,original.begin()+tag.at+tag.size);
        at=tag.at+tag.size;
    }
    plain(length-1);
    if(!written&&!inChoice)append(body);
    out.push_back(0);return true;
}
// Semantic fallbacks only replace the amount/currency phrase. Every instruction,
// dynamic score/time substitution and choice label otherwise stays at its native position.
inline bool unprice(std::span<const uint8_t> original,std::string_view payment,
                    std::vector<uint8_t>& out) {
    std::vector<Tag> tags;size_t length=0;if(!scan(original,tags,length))return false;
    const auto plain=visible(original);const auto folded=lower(plain);
    std::vector<size_t> positions;size_t tagIndex=0;
    for(size_t i=0;i<length-1;) {
        if(tagIndex<tags.size()&&i==tags[tagIndex].at){i+=tags[tagIndex++].size;continue;}
        positions.push_back(i++);
    }
    struct Edit {size_t first,last;std::string value;};std::vector<Edit> edits;
    for(size_t i=0;i<folded.size();) {
        size_t word=0;
        for(std::string_view key:{"rupee","rubin","rubis","rupia","rupie"})if(std::string_view(folded).substr(i).starts_with(key)){word=key.size();break;}
        if(!word){++i;continue;}
        size_t end=i+word;while(end<folded.size()&&folded[end]>='a'&&folded[end]<='z')++end;
        size_t first=i,j=i;while(j>0&&(folded[j-1]==' '||folded[j-1]=='\n'))--j;
        const auto digitsEnd=j;while(j>0&&((folded[j-1]>='0'&&folded[j-1]<='9')||folded[j-1]==','))--j;
        if(j<digitsEnd)first=j;
        const auto beginSentence=folded.find_last_of(".!?\n",first==0?0:first-1);
        const auto context=std::string_view(folded).substr(beginSentence==folded.npos?0:beginSentence+1,end-(beginSentence==folded.npos?0:beginSentence+1));
        const bool prize=context.find("win")!=context.npos||context.find("reward")!=context.npos||context.find("prize")!=context.npos||context.find("give you")!=context.npos;
        edits.push_back({positions[first],positions[end-1]+1,prize?"recognition":std::string(payment)});i=end;
    }
    out.clear();size_t at=0,editIndex=0;tagIndex=0;
    while(at<length-1) {
        while(editIndex<edits.size()&&at>=edits[editIndex].last)++editIndex;
        if(editIndex<edits.size()&&at==edits[editIndex].first)out.insert(out.end(),edits[editIndex].value.begin(),edits[editIndex].value.end());
        if(tagIndex<tags.size()&&at==tags[tagIndex].at) {
            const auto& tag=tags[tagIndex++];
            const bool price=(tag.group==0&&tag.type==50)||(tag.group==5&&tag.type==9);
            if(!price)out.insert(out.end(),original.begin()+at,original.begin()+at+tag.size);
            at+=tag.size;continue;
        }
        if(editIndex>=edits.size()||at<edits[editIndex].first||at>=edits[editIndex].last)out.push_back(original[at]);
        ++at;
    }
    out.push_back(0);return true;
}
struct NativeMessage {uint16_t id;std::span<const uint8_t> text;};
inline uint32_t be32(const uint8_t* p){return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3];}
inline uint16_t be16(const uint8_t* p){return uint16_t(p[0])<<8|p[1];}
// Read-only view of a host-validated BMG. No original game text is shipped by the mod.
inline bool messages(std::span<const uint8_t> bmg,std::vector<NativeMessage>& out) {
    out.clear();if(bmg.size()<0x20||std::memcmp(bmg.data(),"MESGbmg1",8)!=0)return false;
    const auto total=be32(bmg.data()+8),sections=be32(bmg.data()+12);
    if(total<0x20||total>bmg.size()||sections>32)return false;
    std::span<const uint8_t> inf,dat;size_t at=0x20;
    for(unsigned i=0;i<sections;++i) {
        if(at>total||total-at<8)return false;
        const auto size=be32(bmg.data()+at+4);if(size<8||size>total-at)return false;
        if(std::memcmp(bmg.data()+at,"INF1",4)==0){if(!inf.empty())return false;inf=bmg.subspan(at,size);}
        if(std::memcmp(bmg.data()+at,"DAT1",4)==0){if(!dat.empty())return false;dat=bmg.subspan(at+8,size-8);}
        at+=size;
    }
    if(inf.size()<16||dat.empty())return false;
    const auto count=be16(inf.data()+8),stride=be16(inf.data()+10);
    if(stride<20||count>=0x8000||size_t(count)*stride>inf.size()-16)return false;
    for(unsigned i=0;i<count;++i) {
        const auto* entry=inf.data()+16+i*stride;const auto offset=be32(entry);const auto id=be16(entry+4);
        if(offset>=dat.size()||id>=0x8000)return false;
        std::vector<Tag> tags;size_t length=0;if(!scan(dat.subspan(offset),tags,length))return false;
        out.push_back({id,dat.subspan(offset,length)});
    }
    return true;
}
}
