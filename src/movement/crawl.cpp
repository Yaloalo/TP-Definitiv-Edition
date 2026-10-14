// Native crawl speed already follows frame-control animation rate in
// d_a_alink_crawl.inc. Scale the animation once; multiplying movement again
// would square the setting and desynchronise the crawl.
#include "movement/crawl.hpp"
#include "core/tuning.hpp"
#include "core/player.hpp"
#include "services.hpp"
#include "d/actor/d_a_alink.h"
#include <algorithm>
namespace remaster::movement::crawl {
namespace {
DEFINE_HOOK(&daAlink_c::getCrawlMoveAnmSpeed, CrawlAnimationSpeed);
void speed_post(ModContext*,void* args,void* result,void*) {
    auto* link=mods::arg<daAlink_c*>(args,0);
    if (!is_active() || !tuning::get().fastCrawling || !link || !result ||
        link->checkWolf() || !player::input_allowed() || link->checkEventRun() ||
        link->mProcID!=daAlink_c::PROC_CRAWL_MOVE) return;
    *static_cast<float*>(result)*=float(std::clamp<int64_t>(tuning::get().crawlSpeedPercent,50,250))/100;
}
}
ModResult install() { return mods::hook::add_post<CrawlAnimationSpeed>(speed_post); }
}
