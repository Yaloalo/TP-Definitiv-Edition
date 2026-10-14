#pragma once
#include <array>
#include <cstdint>
namespace remaster::economy::dialogue {
enum class Topic {None,Ward,Shield,Hawkeye,Armor,Support,Prayer,Coro,Barnes,Supplies,Challenge,Service};
struct Prompt {uint16_t group,id;Topic topic;};
inline constexpr std::array<Prompt,40> kPrompts={{
    {0,149,Topic::Ward}, // Native Magic Armor get-item description (item 0x30 + 0x65)
    {0,661,Topic::Ward}, // Native equipment description: 0x195 + 0x100
    {2,5155,Topic::Barnes}, // Barnes Special Offer Text
    {2,5809,Topic::Supplies}, // Kakariko Malo Mart Wooden Shield Purchase Confirmation Text
    {2,5810,Topic::Supplies}, // Kakariko Malo Mart Wooden Shield Too Expensive Text
    {2,5813,Topic::Shield}, // Kakariko Malo Mart Hylian Shield Purchase Confirmation Text
    {2,5814,Topic::Shield}, // Kakariko Malo Mart Hylian Shield Too Expensive Text
    {2,5818,Topic::Shield}, // Kakariko Malo Mart Hylian Shield After Purchase Text
    {2,5820,Topic::Hawkeye}, // Kakariko Malo Mart Hawkeye Purchase Confirmation Text
    {2,5821,Topic::Hawkeye}, // Kakariko Malo Mart Hawkeye Too Expensive Text
    {2,5822,Topic::Hawkeye}, // Kakariko Malo Mart Hawkeye After Purchase Text
    {2,5824,Topic::Supplies}, // Kakariko Malo Mart Red Potion Too Expensive Text
    {2,5825,Topic::Supplies}, // Kakariko Malo Mart Red Potion Purchase Confirmation Text
    {2,5990,Topic::Supplies}, // Kakariko Malo Mart Red Potion Text
    {2,5991,Topic::Hawkeye}, // Kakariko Malo Mart Hawkeye Coming Soon Text
    {2,5992,Topic::Hawkeye}, // Kakariko Malo Mart Hawkeye Text
    {2,5996,Topic::Supplies}, // Kakariko Malo Mart Sold Out Text
    {2,5998,Topic::Supplies}, // Kakariko Malo Mart Wooden Shield Text
    {2,5999,Topic::Shield}, // Kakariko Malo Mart Hylian Shield Text
    {4,5413,Topic::Armor}, // Chudleys Shop Magic Armor Text
    {4,5433,Topic::Armor}, // Castle Town Malo Mart Magic Armor After Purchase Text
    {4,5440,Topic::Armor}, // Castle Town Malo Mart Magic Armor Text
    {4,5451,Topic::Armor}, // Castle Town Malo Mart Magic Armor Sold Out Text
    {4,6402,Topic::Prayer}, // Charlo Donation Ask Text
    {4,6403,Topic::Prayer}, // Charlo Donation Choice Text
    {4,6626,Topic::Supplies}, // Castle Town Goron Red Potion Confirmation Initial Text
    {4,6628,Topic::Supplies}, // Castle Town Goron Red Potion Confirmation Later Text
    {4,6614,Topic::Supplies}, // Castle Town Goron Lantern Oil Confirmation Initial Text
    {4,6616,Topic::Supplies}, // Castle Town Goron Lantern Oil Confirmation Later Text
    {4,6650,Topic::Supplies}, // Castle Town Goron Arrow Refill Secret Text
    {4,6651,Topic::Supplies}, // Castle Town Goron Arrow Refill Confirmation Initial Text
    {4,6652,Topic::Supplies}, // Castle Town Goron Arrow Refill Embarrassing Text
    {4,6653,Topic::Supplies}, // Castle Town Goron Arrow Refill Let Me Know Text
    {4,6654,Topic::Supplies}, // Castle Town Goron Arrow Refill Confirmation Later Text
    {4,6662,Topic::Shield}, // Castle Town Goron Hylian Shield Confirmation Initial Text
    {4,6664,Topic::Shield}, // Castle Town Goron Hylian Shield Confirmation Later Text
    {6,5216,Topic::Coro}, // Coro Bottle Offer 1 Text
    {6,5223,Topic::Coro}, // Coro Bottle Offer 2 Text
    {6,5237,Topic::Coro}, // Coro Bottle Offer 3 Text
    {6,5253,Topic::Coro}, // Coro Bottle Offer 4 Text
}};
inline Topic known(uint16_t group,uint16_t id) {for(const auto& r:kPrompts)if(r.group==group&&r.id==id)return r.topic;return Topic::None;}
}
