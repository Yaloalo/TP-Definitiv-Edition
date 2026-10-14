#include "economy/dialogue_model.hpp"
#include "economy/dialogue_table.hpp"
#include <cassert>
#include <iostream>
#include <set>
using namespace remaster::economy::dialogue;
using Bytes=std::vector<uint8_t>;
void text(Bytes& b,const char*s){b.insert(b.end(),s,s+std::strlen(s));}
void tag(Bytes& b,uint8_t group,uint16_t type,std::initializer_list<uint8_t> args={}){b.insert(b.end(),{0x1a,uint8_t(5+args.size()),group,uint8_t(type>>8),uint8_t(type)});b.insert(b.end(),args);}
void be(Bytes& b,size_t at,uint32_t value,unsigned n=4){for(unsigned i=0;i<n;++i)b[at+i]=value>>(8*(n-1-i));}
int main() {
    std::set<std::pair<int,int>> prompts;for(const auto& p:kPrompts)assert(prompts.insert({p.group,p.id}).second);assert(prompts.size()==40);
    Bytes input;text(input,"Pay ");tag(input,255,0,{1});text(input,"1,000 rupees. ");tag(input,0,37,{0,0,0,1});tag(input,0,8,{2});text(input,"Yes\n");tag(input,0,8,{1});text(input,"No");tag(input,0,32);input.push_back(0);
    Bytes output;const char* options[]={"Lend a hand","Another time","Leave"};assert(reword(input,"Offer your help.",options,output));
    auto body=visible(output);assert(body.find("Offer your help.")!=body.npos&&body.find("Lend a hand")!=body.npos&&body.find("rupees")==body.npos);
    std::vector<Tag> before,after;size_t n=0;assert(scan(input,before,n)&&scan(output,after,n)&&before.size()==after.size());
    for(size_t i=0;i<before.size();++i)assert(std::equal(input.begin()+before[i].at,input.begin()+before[i].at+before[i].size,output.begin()+after[i].at)); // selection/input/wait/color controls unchanged, including initial highlight
    Bytes rules;text(rules,"Pay 20 rupees to enter. Break the red pots. Reach ");tag(rules,5,7);text(rules," points!\n");tag(rules,0,9,{1});text(rules,"Lure fishing\n");tag(rules,0,9,{2});text(rules,"Sinking lure\n");tag(rules,0,9,{3});text(rules,"Quit");rules.push_back(0);
    assert(unprice(rules,"no fee",output));body=visible(output);assert(body.find("Pay no fee to enter")!=body.npos&&body.find("Break the red pots")!=body.npos&&body.find("Lure fishing")!=body.npos&&body.find("Sinking lure")!=body.npos&&body.find("rupees")==body.npos);
    assert(scan(rules,before,n)&&scan(output,after,n)&&before.size()==after.size());for(size_t i=0;i<before.size();++i)assert(before[i].group==after[i].group&&before[i].type==after[i].type);
    Bytes many;text(many,"20 rupees 50 rupees! Win 10 rupees. Need 100 rubine? ");many.push_back(0);assert(unprice(many,"no fee",output));body=visible(output);assert(!money(lower(body))&&body.find("recognition")!=body.npos);
    for(size_t i=0;i<input.size();++i){auto prefix=input;prefix.resize(i);Bytes untouched={9};assert(!reword(prefix,"Help",options,untouched)&&untouched==Bytes{9});}
    auto malformed=input;malformed[5]=1;assert(!scan(malformed,before,n)); // invalid tag length
    // A bounded synthetic BMG uses the actual INF1 entry fields and DAT1 string encoding.
    Bytes bmg(32+56+8+input.size()+rules.size());std::memcpy(bmg.data(),"MESGbmg1",8);be(bmg,8,bmg.size());be(bmg,12,2);std::memcpy(bmg.data()+32,"INF1",4);be(bmg,36,56);be(bmg,40,2,2);be(bmg,42,20,2);
    be(bmg,48,0);be(bmg,52,6402,2);be(bmg,68,input.size());be(bmg,72,8002,2);std::memcpy(bmg.data()+88,"DAT1",4);be(bmg,92,bmg.size()-88);std::copy(input.begin(),input.end(),bmg.begin()+96);std::copy(rules.begin(),rules.end(),bmg.begin()+96+input.size());
    std::vector<NativeMessage> messagesFound;assert(messages(bmg,messagesFound)&&messagesFound.size()==2&&messagesFound[0].id==6402&&messagesFound[1].id==8002);
    for(size_t i=0;i<bmg.size();++i){auto prefix=bmg;prefix.resize(i);assert(!messages(prefix,messagesFound));}
    auto bad=bmg;be(bad,48,0xffffffff);assert(!messages(bad,messagesFound));bad=bmg;be(bad,36,4);assert(!messages(bad,messagesFound));bad=bmg;be(bad,42,19,2);assert(!messages(bad,messagesFound));
    std::cout<<"Native dialogue codec: complete controls/choices, intact rules/score tags, price phrases and bounded BMG parsing passed\n";
}
